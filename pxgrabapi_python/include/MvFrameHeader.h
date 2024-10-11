#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>



enum PxMvVideoMetadataType {
	/**
	 * 매 키 프레임에 삽입되는 데이터입니다. 키 프레임 개념이 없을 경우 적당한 주기로 삽입하면 됩니다.
	 * 카메라 파라미터 정보 등이 삽입됩니다.
	 *
	 * # Protocol
	 *
	 * ```jsonc
	 * {
	 *     "version": 2,
	 *
	 *     "dev-name": "string", // 장치 이름
	 *
	 *     "vendor": "string", // 장치 벤더
	 *
	 *     "calib?": {
	 *         "intr_id": 0, // 0이면 intr 없음, 1 이상이면 있음.
	 *         "intr_ty?": "pinhole|fisheye",
	 *         "intr?": "base64string",
	 *
	 *         "extr_id": 0, // 0이면 extr 없음, 1 이상이면 있음.
	 *         "extr_ty?": "ocv",
	 *         "extr?": "base64string",
	 *     }
	 * }
	 * ```
	 *
	 * ## Camera Paramter Layout
	 *
	 * 모든 변수는 double precision floating point이며, paddingless packed binary의
	 * base64 representation
	 *
	 * ### Intrinsic
	 * - 'pinhole'
	 *     - 'fx', 'fy', 'cx', 'cy', 'k1', 'k2', 'p1', 'p2', 'k3'
	 * - 'fisheye'
	 *     - 'fx', 'fy', 'cx', 'cy', 'k1', 'k2', 'k3', 'k4'
	 *
	 * ### Extrinsic
	 * - 'ocv'
	 *     - `rvec[3]`, `tvec[3]`
	 *
	 */
	PXMV_VIDEO_METADATA_KEY_FRAME_EX = 1,
	/**
	 * 매 프레임 삽입되는 데이터 중, 간단한 데이터만 포함합니다:
	 *
	 * {
	 *     "version": 2,
	 *     "host_utc_us": 1234567890,
	 *     "channel": "channel-name"
	 * }
	 */
	PXMV_VIDEO_METADATA_FRAME = 2,
	/**
	 * 원본 장치의 상태를 복원하기 위한 모든 정보를 포함합니다.
	 * 만약 카메라 파라미터가 키 프레임 사이에 변경된 경우, 카메라 파라미터도 포함할 수 있습니다.
	 *
	 * 레거시 프로토콜의 FRAME_EX에 해당.
	 *
	 * ```json
	 * {
	 *     "version": 2,
	 *
	 *     "host_utc_us": 1234567890,
	 *     "dev_ts_us": 1234567890,
	 *     "dev_utc_us": 1234567890,
	 *     "frame_number": 1234567890,
	 *
	 *     "offset": [0, 0]
	 *     // Width, Height는 이미지 자체에 포함되는 정보
	 *
	 *     "calib?": {
	 *         // SAME AS ABOVE
	 *     }
	 * }
	 * ```
	 */
	PXMV_VIDEO_METADATA_FRAME_EX = 3,
};


enum PxMvCameraModel {
	PXMV_CAMERA_MODEL_NONE = 0,
	PXMV_CAMERA_MODEL_OPENCV = 1,
	PXMV_CAMERA_MODEL_OPENCV_FISHEYE = 2,
};

struct PxMvCameraModelOCV {
	double fx;
	double fy;
	double cx;
	double cy;
	double k1;
	double k2;
	double p1;
	double p2;
	double k3;
};

struct PxMvCameraModelOCVFishEye {
	double fx;
	double fy;
	double cx;
	double cy;
	double k1;
	double k2;
	double k3;
	double k4;
};

union PxMvCameraIntrinsicUnion {
	uint64_t _max_size[16];
	struct PxMvCameraModelOCV cv;
	struct PxMvCameraModelOCVFishEye fisheye;
};

struct PxMvCameraExtrinsic {
	double rvec[3];
	double tvec[3];
	uint64_t _reserved[4];
};

typedef struct _PxMvCameraParameter {
	enum PxMvCameraModel camera_model;
	uint32_t _reserved0;
	uint64_t intrinsic_id = 0;
	uint64_t extrinsic_id = 0;
	union PxMvCameraIntrinsicUnion intrinsic;
	struct PxMvCameraExtrinsic extrinsic;
	uint64_t _reserved1[3];
} PxMvCameraParameter;


enum PxMvDataCategory {
	PXMV_DATA_CATEGORY_UNSPECIFIED = 0,
	PXMV_DATA_CATEGORY_VIDEO = 1,
	PXMV_DATA_CATEGORY_AUDIO = 16,
	PXMV_DATA_CATEGORY_SENSOR_1D = 30,
	PXMV_DATA_CATEGORY_OTHER = 63,
};


// Indicate if pixel is monochrome or RGB
#define PX_GVSP_PIX_MONO                                0x01000000 
#define PX_GVSP_PIX_RGB                                 0x02000000  
#define PX_GVSP_PIX_COLOR                               0x02000000
#define PX_GVSP_PIX_CUSTOM                              0x80000000
#define PX_GVSP_PIX_COLOR_MASK                          0xFF000000
// Indicate effective number of bits occupied by the pixel (including padding).
// This can be used to compute amount of memory required to store an image.
#define PX_PIXEL_BIT_COUNT(n)                           ((n) << 16)
#define PX_GVSP_PIX_EFFECTIVE_PIXEL_SIZE_MASK           0x00FF0000
#define PX_GVSP_PIX_EFFECTIVE_PIXEL_SIZE_SHIFT          16
// Pixel ID: lower 16-bit of the pixel formats
#define PX_GVSP_PIX_ID_MASK                             0x0000FFFF
#define PX_GVSP_PIX_COUNT                               0x46 // next Pixel ID available

enum PxPixelType
{
	// Undefined pixel type
#ifdef WIN32
	PX_PixelType_Gvsp_Undefined = 0xFFFFFFFF,

#else
	PX_PixelType_Gvsp_Undefined = -1,
#endif

	// Mono buffer format defines
	PX_PixelType_Gvsp_Mono1p = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(1) | 0x0037),
	PX_PixelType_Gvsp_Mono2p = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(2) | 0x0038),
	PX_PixelType_Gvsp_Mono4p = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(4) | 0x0039),
	PX_PixelType_Gvsp_Mono8 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0001),
	PX_PixelType_Gvsp_Mono8_Signed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0002),
	PX_PixelType_Gvsp_Mono10 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0003),
	PX_PixelType_Gvsp_Mono10_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0004),
	PX_PixelType_Gvsp_Mono12 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0005),
	PX_PixelType_Gvsp_Mono12_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0006),
	PX_PixelType_Gvsp_Mono14 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0025),
	PX_PixelType_Gvsp_Mono16 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0007),

	// Bayer buffer format defines 
	PX_PixelType_Gvsp_BayerGR8 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0008),
	PX_PixelType_Gvsp_BayerRG8 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0009),
	PX_PixelType_Gvsp_BayerGB8 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x000A),
	PX_PixelType_Gvsp_BayerBG8 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x000B),
	PX_PixelType_Gvsp_BayerRBGG8 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0046),
	PX_PixelType_Gvsp_BayerGR10 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000C),
	PX_PixelType_Gvsp_BayerRG10 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000D),
	PX_PixelType_Gvsp_BayerGB10 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000E),
	PX_PixelType_Gvsp_BayerBG10 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000F),
	PX_PixelType_Gvsp_BayerGR12 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0010),
	PX_PixelType_Gvsp_BayerRG12 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0011),
	PX_PixelType_Gvsp_BayerGB12 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0012),
	PX_PixelType_Gvsp_BayerBG12 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0013),
	PX_PixelType_Gvsp_BayerGR10_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0026),
	PX_PixelType_Gvsp_BayerRG10_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0027),
	PX_PixelType_Gvsp_BayerGB10_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0028),
	PX_PixelType_Gvsp_BayerBG10_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0029),
	PX_PixelType_Gvsp_BayerGR12_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002A),
	PX_PixelType_Gvsp_BayerRG12_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002B),
	PX_PixelType_Gvsp_BayerGB12_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002C),
	PX_PixelType_Gvsp_BayerBG12_Packed = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002D),
	PX_PixelType_Gvsp_BayerGR16 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x002E),
	PX_PixelType_Gvsp_BayerRG16 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x002F),
	PX_PixelType_Gvsp_BayerGB16 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0030),
	PX_PixelType_Gvsp_BayerBG16 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0031),

	// RGB Packed buffer format defines 
	PX_PixelType_Gvsp_RGB8_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x0014),
	PX_PixelType_Gvsp_BGR8_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x0015),
	PX_PixelType_Gvsp_RGBA8_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(32) | 0x0016),
	PX_PixelType_Gvsp_BGRA8_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(32) | 0x0017),
	PX_PixelType_Gvsp_RGB10_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x0018),
	PX_PixelType_Gvsp_BGR10_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x0019),
	PX_PixelType_Gvsp_RGB12_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x001A),
	PX_PixelType_Gvsp_BGR12_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x001B),
	PX_PixelType_Gvsp_RGB16_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x0033),
	PX_PixelType_Gvsp_BGR16_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x004B),
	PX_PixelType_Gvsp_RGBA16_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x0064),
	PX_PixelType_Gvsp_BGRA16_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x0051),
	PX_PixelType_Gvsp_RGB10V1_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(32) | 0x001C),
	PX_PixelType_Gvsp_RGB10V2_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(32) | 0x001D),
	PX_PixelType_Gvsp_RGB12V1_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(36) | 0X0034),
	PX_PixelType_Gvsp_RGB565_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x0035),
	PX_PixelType_Gvsp_BGR565_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0X0036),

	// YUV Packed buffer format defines 
	PX_PixelType_Gvsp_YUV411_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(12) | 0x001E),
	PX_PixelType_Gvsp_YUV422_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x001F),
	PX_PixelType_Gvsp_YUV422_YUYV_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x0032),
	PX_PixelType_Gvsp_YUV444_Packed = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x0020),
	PX_PixelType_Gvsp_YCBCR8_CBYCR = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x003A),
	PX_PixelType_Gvsp_YCBCR422_8 = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x003B),
	PX_PixelType_Gvsp_YCBCR422_8_CBYCRY = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x0043),
	PX_PixelType_Gvsp_YCBCR411_8_CBYYCRYY = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(12) | 0x003C),
	PX_PixelType_Gvsp_YCBCR601_8_CBYCR = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x003D),
	PX_PixelType_Gvsp_YCBCR601_422_8 = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x003E),
	PX_PixelType_Gvsp_YCBCR601_422_8_CBYCRY = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x0044),
	PX_PixelType_Gvsp_YCBCR601_411_8_CBYYCRYY = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(12) | 0x003F),
	PX_PixelType_Gvsp_YCBCR709_8_CBYCR = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x0040),
	PX_PixelType_Gvsp_YCBCR709_422_8 = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x0041),
	PX_PixelType_Gvsp_YCBCR709_422_8_CBYCRY = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x0045),
	PX_PixelType_Gvsp_YCBCR709_411_8_CBYYCRYY = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(12) | 0x0042),

	// YUV420
	PX_PixelType_Gvsp_YUV420SP_NV12 = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(12) | 0x8001),
	PX_PixelType_Gvsp_YUV420SP_NV21 = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(12) | 0x8002),

	// RGB Planar buffer format defines 
	PX_PixelType_Gvsp_RGB8_Planar = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x0021),
	PX_PixelType_Gvsp_RGB10_Planar = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x0022),
	PX_PixelType_Gvsp_RGB12_Planar = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x0023),
	PX_PixelType_Gvsp_RGB16_Planar = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x0024),

	// 
	PX_PixelType_Gvsp_Jpeg = (PX_GVSP_PIX_CUSTOM | PX_PIXEL_BIT_COUNT(24) | 0x0001),

	PX_PixelType_Gvsp_Coord3D_ABC32f = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(96) | 0x00C0),//0x026000C0
	PX_PixelType_Gvsp_Coord3D_ABC32f_Planar = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(96) | 0x00C1),//0x026000C1

	//  PX_PixelType_Gvsp_Coord3D_AC32f_64; the value is discarded
	PX_PixelType_Gvsp_Coord3D_AC32f = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(40) | 0x00C2),
	//  the value is discarded     
	PX_PixelType_Gvsp_COORD3D_DEPTH_PLUS_MASK = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(28) | 0x0001),

	PX_PixelType_Gvsp_Coord3D_ABC32 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(96) | 0x3001),//0x82603001
	PX_PixelType_Gvsp_Coord3D_AB32f = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x3002),//0x82403002
	PX_PixelType_Gvsp_Coord3D_AB32 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x3003),//0x82403003
	PX_PixelType_Gvsp_Coord3D_AC32f_64 = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x00C2),//0x024000C2
	PX_PixelType_Gvsp_Coord3D_AC32f_Planar = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x00C3),//0x024000C3
	PX_PixelType_Gvsp_Coord3D_AC32 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x3004),//0x82403004
	PX_PixelType_Gvsp_Coord3D_A32f = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(32) | 0x00BD),//0x012000BD
	PX_PixelType_Gvsp_Coord3D_A32 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(32) | 0x3005),//0x81203005
	PX_PixelType_Gvsp_Coord3D_C32f = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(32) | 0x00BF),//0x012000BF
	PX_PixelType_Gvsp_Coord3D_C32 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(32) | 0x3006),//0x81203006
	PX_PixelType_Gvsp_Coord3D_ABC16 = (PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x00B9),//0x023000B9
	PX_PixelType_Gvsp_Coord3D_C16 = (PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x00B8),//0x011000B8

	PX_PixelType_Gvsp_Float32 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(32) | 0x0001),//0x81200001

	PX_PixelType_Gvsp_HB_Mono8 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0001),
	PX_PixelType_Gvsp_HB_Mono10 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0003),
	PX_PixelType_Gvsp_HB_Mono10_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0004),
	PX_PixelType_Gvsp_HB_Mono12 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0005),
	PX_PixelType_Gvsp_HB_Mono12_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0006),
	PX_PixelType_Gvsp_HB_Mono16 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0007),
	PX_PixelType_Gvsp_HB_BayerGR8 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0008),
	PX_PixelType_Gvsp_HB_BayerRG8 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0009),
	PX_PixelType_Gvsp_HB_BayerGB8 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x000A),
	PX_PixelType_Gvsp_HB_BayerBG8 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x000B),
	PX_PixelType_Gvsp_HB_BayerRBGG8 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(8) | 0x0046),
	PX_PixelType_Gvsp_HB_BayerGR10 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000C),
	PX_PixelType_Gvsp_HB_BayerRG10 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000D),
	PX_PixelType_Gvsp_HB_BayerGB10 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000E),
	PX_PixelType_Gvsp_HB_BayerBG10 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x000F),
	PX_PixelType_Gvsp_HB_BayerGR12 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0010),
	PX_PixelType_Gvsp_HB_BayerRG12 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0011),
	PX_PixelType_Gvsp_HB_BayerGB12 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0012),
	PX_PixelType_Gvsp_HB_BayerBG12 = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(16) | 0x0013),
	PX_PixelType_Gvsp_HB_BayerGR10_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0026),
	PX_PixelType_Gvsp_HB_BayerRG10_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0027),
	PX_PixelType_Gvsp_HB_BayerGB10_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0028),
	PX_PixelType_Gvsp_HB_BayerBG10_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x0029),
	PX_PixelType_Gvsp_HB_BayerGR12_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002A),
	PX_PixelType_Gvsp_HB_BayerRG12_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002B),
	PX_PixelType_Gvsp_HB_BayerGB12_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002C),
	PX_PixelType_Gvsp_HB_BayerBG12_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_MONO | PX_PIXEL_BIT_COUNT(12) | 0x002D),
	PX_PixelType_Gvsp_HB_YUV422_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x001F),
	PX_PixelType_Gvsp_HB_YUV422_YUYV_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(16) | 0x0032),
	PX_PixelType_Gvsp_HB_RGB8_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x0014),
	PX_PixelType_Gvsp_HB_BGR8_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(24) | 0x0015),
	PX_PixelType_Gvsp_HB_RGBA8_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(32) | 0x0016),
	PX_PixelType_Gvsp_HB_BGRA8_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(32) | 0x0017),
	PX_PixelType_Gvsp_HB_RGB16_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x0033),
	PX_PixelType_Gvsp_HB_BGR16_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(48) | 0x004B),
	PX_PixelType_Gvsp_HB_RGBA16_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x0064),
	PX_PixelType_Gvsp_HB_BGRA16_Packed = (PX_GVSP_PIX_CUSTOM | PX_GVSP_PIX_COLOR | PX_PIXEL_BIT_COUNT(64) | 0x0051),

	PX_PixelType_H264_YUV420P,
	PX_PixelType_JPEG,
};

typedef struct _PxMvDeviceInfo {
	int32_t					nWidth;
	int32_t					nHeight;
	char					channelName[16];
	uint64_t                name_hash;
	char					vendor[16];
	enum PxPixelType        enPixelType;
	double					fps;

	PxMvCameraParameter   camera_parameter;

} PxMvDeviceInfo;

static const uint32_t MV_FRAME_INFO_VERSION = 1;

#pragma pack(push, 1)
typedef struct _MV_FRAME_INFO {
	uint8_t 				start_code[4] = { 0x3F, 0xA7, 0xA4, 0x42 };
	uint32_t				header_size = sizeof(struct _MV_FRAME_INFO);
	uint32_t				version = MV_FRAME_INFO_VERSION;

	uint64_t				nFrameNum = 0;
	uint64_t				nHWFrameNum = 0;

	uint64_t				utc_timestamp_us = 0;
	uint64_t				hw_timestamp_us = 0;

	int32_t					nOffsetX = 0;
	int32_t					nOffsetY = 0;

	PxMvDeviceInfo		    deviceInfo = { 0, };

	uint64_t                nLostPacket = 0;

	int32_t					nFrameLen = 0;

} MV_FRAME_INFO;
#pragma pack(pop)