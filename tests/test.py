from videoclientapi_python.client import GrabClient
import time
import argparse

def main():
    from videoclientapi_python.utils import Visualizer
    global visualizer
    visualizer = Visualizer()

    parser = argparse.ArgumentParser()
    parser.add_argument("--colorspace")
    parser.add_argument("--save-path")
    args = parser.parse_args()

    save_path = args.save_path
    def cb(param):
        global visualizer
        print(visualizer.get_info_string(param))
        # visualizer.save_frame_as_image(param, save_path=save_path)
        # visualizer.save_frame_as_image(param, save_path="/home/kwon/Downloads/images")
        # pass

    client = GrabClient(callback=cb,
                        host="127.0.0.1",
                        port=31000,
                        devices=["MV-GTL-DEV-001"],
                        gpu_index=0,
                        fps=30,
                        verbose=True,
                        # colorspace="rgb")
                        colorspace=args.colorspace,)
    client.start_consumming()


if __name__ == "__main__":
    main()
