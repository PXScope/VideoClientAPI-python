import copy
import cv2
from pathlib import Path


class Visualizer:
    def __init__(self):
        self.n = 0

    @staticmethod
    def get_info_string(buffer):
        info = copy.deepcopy(buffer)
        oss = ""
        oss += "\n[Frame Info]\n"
        oss += "- frame_num: " + str(info.package_number) + "\n"
        # oss += "- timestamp: " + str(info.timestamp) + "\n"
        # oss += "- frame_key: " + str(info.package_key) + "\n"
        #
        # oss += "- intr_id: " + str(info.intrinsic.id) + "\n"
        # oss += "- fx_fy_cx_cy: " + str(info.intrinsic.fx_fy_cx_cy)
        # oss += ", distortion: " + str(info.intrinsic.distortion)
        # oss += "\n"
        #
        # oss += "- extr_id: " + str(info.extrinsic.id) + "\n"
        # oss += "- rvec [ " + str(info.extrinsic.rvec)
        # oss += ", tvec [ " + str(info.extrinsic.tvec) + "]"

        return oss

    def save_frame_as_image(self, buffer, save_path):
        Path(save_path).mkdir(parents=True, exist_ok=True)
        # 이미지 파일로 저장
        filename = save_path + "/" + str(buffer.package_number) + ".png"
        image = buffer.image
        if not cv2.imwrite(filename, image[:, :, ::-1]):
            print("Failed to save image")
        else:
            self.n += 1
