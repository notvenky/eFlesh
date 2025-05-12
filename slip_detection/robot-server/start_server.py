import hydra
from initializers import StartServer
import signal
import sys

@hydra.main(config_path="configs", config_name="start_server", version_base="1.2")
def main(cfg):
    """
    Main function to start the server
    """
    #Start the server
    server = StartServer(cfg)
    processes = server.get_processes()

    for process in processes:
        process.start()
    for process in processes:
        process.join()
    print("Server started")

    def signal_handler(sig, frame):
        for process in processes:
            process.terminate()
        sys.exit(0)
    signal.signal(signal.SIGINT, signal_handler)

if __name__ == "__main__":
    main()