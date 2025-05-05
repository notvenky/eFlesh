import argparse
import os
import csv
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
import matplotlib.pyplot as plt
from model import MLP


class SensorForceDataset(torch.utils.data.Dataset):
    def __init__(self, states_csv, sensor_csv, normalize=True):
        self.states_time, self.states_weight = [], []
        with open(states_csv) as f:
            for row in csv.reader(f):
                vals = [float(x) for x in row]
                if vals[4] != -1:
                    self.states_time.append(vals[0])
                    self.states_weight.append(vals[4])

        self.sensor_time, self.sensor_data = [], []
        with open(sensor_csv) as f:
            for row in csv.reader(f):
                vals = [float(x) for x in row]
                self.sensor_time.append(vals[0])
                self.sensor_data.append(vals[1:])

        self.states_time = np.array(self.states_time)
        self.states_weight = np.array(self.states_weight)
        self.sensor_time = np.array(self.sensor_time)
        self.sensor_data = np.array(self.sensor_data)

        matched_sens, matched_w = [], []
        for i in range(len(self.sensor_time)):
            t = self.sensor_time[i]
            idx = np.argmin(np.abs(self.states_time - t))
            matched_sens.append(self.sensor_data[i])
            matched_w.append(self.states_weight[idx])

        self.X = np.array(matched_sens)
        self.Y = np.array(matched_w).reshape(-1, 1)
        self.normalize = normalize
        if normalize:
            self.x_mean = self.X.mean(axis=0)
            self.x_std = self.X.std(axis=0)
            self.y_mean = self.Y.mean(axis=0)
            self.y_std = self.Y.std(axis=0)
            self.x_std[self.x_std < 1e-8] = 1
            self.y_std[self.y_std < 1e-8] = 1
            self.X = (self.X - self.x_mean) / self.x_std
            self.Y = (self.Y - self.y_mean) / self.y_std

    def __len__(self):
        return len(self.X)

    def __getitem__(self, idx):
        return self.X[idx], self.Y[idx]

    def unnormalize_y(self, y):
        if not self.normalize:
            return y
        if isinstance(y, torch.Tensor):
            return y * torch.tensor(self.y_std, device=y.device) + torch.tensor(self.y_mean, device=y.device)
        else:
            return y * self.y_std + self.y_mean


class SensorSpatialDataset(torch.utils.data.Dataset):
    def __init__(self, states_csv, sensor_csv, normalize_targets=True):
        self.states_time, self.states_xyz = [], []
        with open(states_csv) as f:
            for row in csv.reader(f):
                row = [float(v) for v in row]
                self.states_time.append(row[0])
                self.states_xyz.append(row[1:4])
        self.states_time = np.array(self.states_time)
        self.states_xyz = np.array(self.states_xyz)

        self.sensor_time, self.sensor_data = [], []
        with open(sensor_csv) as f:
            for row in csv.reader(f):
                row = [float(v) for v in row]
                self.sensor_time.append(row[0])
                self.sensor_data.append(row[1:])
        self.sensor_time = np.array(self.sensor_time)
        self.sensor_data = np.array(self.sensor_data)

        matched_xyz, matched_sens, matched_mask = [], [], []
        for i in range(len(self.sensor_time)):
            st = self.sensor_time[i]
            idx = np.argmin(np.abs(self.states_time - st))
            matched_xyz.append(self.states_xyz[idx])
            matched_sens.append(self.sensor_data[i])
            matched_mask.append(self.states_xyz[idx][2] < 145.1)

        matched_xyz = np.array(matched_xyz)
        matched_sens = np.array(matched_sens)
        matched_mask = np.array(matched_mask)

        self.X = matched_sens[matched_mask]
        self.Y = matched_xyz[matched_mask]

        self.normalize_targets = normalize_targets
        if normalize_targets:
            self.y_mean = self.Y.mean(axis=0)
            self.y_std = self.Y.std(axis=0)
            self.y_std[self.y_std < 1e-8] = 1.0
            self.Y = (self.Y - self.y_mean) / self.y_std
        else:
            self.y_mean = np.zeros(3)
            self.y_std = np.ones(3)

    def __len__(self):
        return len(self.X)

    def __getitem__(self, idx):
        return self.X[idx], self.Y[idx]

    def unnormalize_y(self, y):
        if not self.normalize_targets:
            return y
        if isinstance(y, torch.Tensor):
            return y * torch.tensor(self.y_std, device=y.device) + torch.tensor(self.y_mean, device=y.device)
        else:
            return y * self.y_std + self.y_mean


def train_model(dataset, model, epochs, batch_size, out_dim, device):
    n = len(dataset)
    idxs = np.arange(n)
    split = int(0.8 * n)
    train_idx, val_idx = idxs[:split], idxs[split:]

    train_loader = torch.utils.data.DataLoader(torch.utils.data.Subset(dataset, train_idx), batch_size=batch_size, shuffle=True)
    val_loader = torch.utils.data.DataLoader(torch.utils.data.Subset(dataset, val_idx), batch_size=batch_size, shuffle=False)

    optimizer = optim.Adam(model.parameters(), lr=1e-3)
    criterion = nn.MSELoss()

    for e in range(epochs):
        model.train()
        for Xb, Yb in train_loader:
            Xb, Yb = Xb.float().to(device), Yb.float().to(device)
            optimizer.zero_grad()
            pred = model(Xb)
            loss = criterion(pred, Yb)
            loss.backward()
            optimizer.step()

        model.eval()
        total_sq = 0
        with torch.no_grad():
            for Xb, Yb in val_loader:
                Xb, Yb = Xb.float().to(device), Yb.float().to(device)
                pred = model(Xb)
                diff = pred - Yb
                total_sq += torch.sum(diff**2).item()
        rmse = np.sqrt(total_sq / len(val_idx))
        print(f"Epoch {e+1}/{epochs} | Val RMSE: {rmse:.3f}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=["spatial", "normal", "shear"], required=True)
    parser.add_argument("--folder", type=str, required=True)
    parser.add_argument("--epochs", type=int, default=1000)
    parser.add_argument("--batch_size", type=int, default=64)
    args = parser.parse_args()

    states_csv = os.path.join(args.folder, "states.csv")
    sensor_csv = os.path.join(args.folder, "sensor_post_baselines.csv")

    if args.mode == "spatial":
        dataset = SensorSpatialDataset(states_csv, sensor_csv)
        out_dim = 3
    else:
        dataset = SensorForceDataset(states_csv, sensor_csv)
        out_dim = 1

    in_dim = dataset.X.shape[1]
    model = MLP(in_dim=in_dim, out_dim=out_dim).to(torch.device("cuda" if torch.cuda.is_available() else "cpu"))
    train_model(dataset, model, args.epochs, args.batch_size, out_dim, model.device)


if __name__ == "__main__":
    main()