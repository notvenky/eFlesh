import torch
from torch import nn

class LSTMNet(nn.Module):
  def __init__(self, input_size, hidden_size, lstm_hidden_size, output_size, nlayers):
    super(LSTMNet, self).__init__()
    self.embed = nn.Linear(input_size, hidden_size)
    self.lstm = torch.nn.LSTM(hidden_size, lstm_hidden_size, batch_first=True, num_layers=nlayers)
    self.decoder = nn.Linear(lstm_hidden_size, output_size)

  def forward(self, x):
    embed = self.embed(x)
    out, _ = self.lstm(embed.reshape((-1, *embed.shape[-2:])))
    out = self.decoder(out[:, -1])
    return out.flatten()