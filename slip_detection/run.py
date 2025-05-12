import hydra
from omegaconf import OmegaConf

import pickle
from sklearn.linear_model import LogisticRegression  # noqa
from robot.controller import Controller


def init_model(cfg: OmegaConf):
    with open(f"checkpoints/handover/{cfg.model_pkl}", "rb") as f:
        model = pickle.load(f)

    return model


def run(cfg: OmegaConf):
    model = init_model(cfg)
    dict_cfg = OmegaConf.to_container(cfg, resolve=True)

    controller = Controller(cfg=dict_cfg)
    controller.setup_model(model)
    controller.run()


@hydra.main(version_base="1.2", config_name="run_handover_demo", config_path="configs")
def main(cfg: OmegaConf):
    run(cfg)


if __name__ == "__main__":
    main()
