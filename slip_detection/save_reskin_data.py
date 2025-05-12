from reskin_server import ReskinSensorSubscriber
import torch


def save_reskin_data():
    reskin_subscriber = ReskinSensorSubscriber()
    sensor_data = []
    try:
        while True:
            reskin_state = reskin_subscriber.get_sensor_state()
            sensor_data.append(reskin_state["sensor_values"])
    except KeyboardInterrupt:
        pass
    finally:
        torch.save(torch.tensor(sensor_data), "reskin_data.pt")


if __name__ == "__main__":
    save_reskin_data()
