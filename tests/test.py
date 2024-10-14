from videoclientapi_python.client import GrabClient


def main():
    from videoclientapi_python.utils import Visualizer
    global visualizer
    visualizer = Visualizer()

    def cb(param):
        global visualizer
        print(visualizer.get_info_string(param))
        visualizer.save_frame_as_image(param, save_path="/home/kwon/Downloads/images")

    client = GrabClient(callback=cb,
                        host="127.0.0.1",
                        port=31000,
                        devices=["MV-GTL-DEV-001"],
                        fps=30,
                        colorspace="rgb")
    client.start_consumming()


if __name__ == "__main__":
    main()