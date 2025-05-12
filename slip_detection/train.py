import torch
from torch import nn, Tensor
from pathlib import Path
from scipy.signal import savgol_filter
import numpy as np


def get_features_from_window(seq, baseline):
    features = seq[:, 15:] - baseline
    return features.flatten()

class Dataset:
    def __init__(self, path: str, window_len: int, pull_len: int, normalize_max: float):
        self.path = Path(path)
        self.files = sorted(
            self.path.glob("seq_*.pt"),
            key=lambda x: int(str(x).split("_")[-1].split(".")[0]),
        )
        self.seqs = [torch.load(file, weights_only=True) for file in self.files]
        with open(self.path / "labels", "r") as f:
            self.labels = [int(start) for start in f.readlines()]

        # collect features (0.1s segments concatenated)
        # until <label>, no pull
        # between <label>-<label+pull_duration_s>, pull

        xy_mask = [0, 1, 3, 4, 6, 7, 9, 10, 12, 13]
        samples = []
        for seq, label in zip(self.seqs, self.labels):
            seq = savgol_filter(seq.numpy(), 11, 3, axis=0)
            baseline = np.median(seq[:50, 15:], axis=0, keepdims=True)

            # Option A: Just one sample from the entire sequence:
            features = seq[:, 15:] - baseline  # entire seq
            xy_mask = [0, 1, 3, 4, 6, 7, 9, 10, 12, 13]
            features = features[:, xy_mask].flatten()
            samples.append((features / normalize_max, label)) 

        self.samples = samples
        self.normalize_constant = normalize_max

    def __getitem__(self, i):
        return self.samples[i]

    def __len__(self):
        return len(self.samples)


class Model(nn.Module):
    def __init__(self, window_len: int):
        super().__init__()
        self.window_len = window_len
        self.linear = nn.Linear(window_len * 15, 1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        # x: (batch, window_len * 15)
        assert x.shape[1] == self.window_len * 15
        return self.linear(x)

def evaluate(model: nn.Module, loader, device: str):
    model.eval()
    correct = 0
    total = 0
    with torch.no_grad():
        for x, y in loader:
            x, y = x.to(device), y.to(device).float()
            y_pred = model(x).flatten()
            correct += ((y_pred > 0) == y).sum().item()
            total += len(y)
    return correct / total

def train(path="data", window_len: int = 10, pull_len: int = 30, normalize_max: float = 100.0, device: str = "cpu"):
    train_dataset = Dataset(path=path, window_len=window_len, pull_len=pull_len, normalize_max=normalize_max)
    test_dataset = Dataset(path=path, window_len=window_len, pull_len=pull_len, normalize_max=normalize_max)

    train_loader = torch.utils.data.DataLoader(
        train_dataset, batch_size=32, shuffle=True
    )
    test_loader = torch.utils.data.DataLoader(test_dataset, batch_size=32, shuffle=False)

    model = Model(window_len=window_len)
    model.to(device)

    optimizer = torch.optim.Adam(model.parameters(), lr=1e-3)
    criterion = nn.BCEWithLogitsLoss()

    # print number of parameters in millions
    print(sum(p.numel() for p in model.parameters()) / 1_000_000, "M parameters")

    max_accuracy = 0
    for epoch in range(100):
        model.train()
        for x, y in train_loader:
            x, y = x.to(device), y.to(device).float()
            optimizer.zero_grad()
            y_pred = model(x).flatten()
            loss: Tensor = criterion(y_pred, y)
            loss.backward()
            optimizer.step()

        model.eval()
        print(f"Epoch {epoch}, train_loss: {loss.item()}")
        accuracy = evaluate(model, test_loader, device)
        if accuracy > max_accuracy:
            max_accuracy = accuracy
            torch.save(model.state_dict(), "checkpoints/new_data_linear_model.pt")
        print(f"Epoch {epoch}, test_accuracy: {accuracy}")


if __name__ == "__main__":
    train()
