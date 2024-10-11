#ifndef VIDEO_CLIENT_API_H
#define VIDEO_CLIENT_API_H

//#include "api_common.h"
#include <iostream>
#include <sstream>


#if defined(_WIN32) && defined(_USRDLL)
#define API_CALL __cdecl
#if defined(VIDEO_CLIENT_API_EXPORTS)
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT __declspec(dllimport)
#endif // #if defined(VIDEO_CLIENT_API_EXPORTS)
#else
#define API_CALL
#define API_EXPORT 
#endif // #if defined(_WIN32) && defined(_USRDLL)


#ifdef __cplusplus
extern "C" {
#endif


void Assert_Throw(int failed, const char* exp, const char* func, const char* file, int line, const char* str) {
	try {
		if (failed) {
			std::ostringstream oss;
			oss << "Assertion failed: (" << exp;
			if (str && *str) {
				oss << ", " << str;
			}
			oss << "), function " << func << ", file " << file << ", line " << line << "." << std::endl;
			throw std::runtime_error(oss.str());
		}
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}



typedef enum {
	SUCCESS = 0,
	INVALID_CLIENT_CONTEXT,
	INVALID_URL,
	CONNECT_TIMEOUT,
	CALLBACK_NOT_SET,
	INVALID_GPU_INDEX,
	INIT_VIDEO_PROCESSOR_FAILED,
	INIT_VIDEO_DECODER_FAILED,

} api_err_t;

typedef enum {
    avc = 0,  ///< Advanced Video Coding (H.264)
    hevc = 1, ///< High Efficiency Video Coding (H.265)
} codec_t;


typedef enum {
	none = 0,   ///< Same as input format
	mono,       ///< Monochrome (grayscale)
	rgb24,      ///< 24-bit RGB color
	bgr24,      ///< 24-bit BGR color
} pixel_format_t;

/**
 * @brief Structure for video processing context.
 * 
 * This structure holds the settings for video processing, including:
 * - gpu_index: Index of the GPU to be used for video processing.
 * - target_format: Determines the pixel format of the video frames in the callback function.
 * - target_fps: Determines the frequency of the video frames in the callback function.
 */
struct videoproc_context {
    int gpu_index;               	///< Index of the GPU to be used for video processing
    pixel_format_t target_format; 	///< The pixel format of the video frames in the callback function.
    int target_fps;              	///< The frequency of the video frames in the callback function.
};


typedef struct video_client_t* video_client;

/**
 * @brief Callback type for handling disconnection events.
 * 
 * This callback function is called when the video client is disconnected from the video server.
 * 
 * @param ctx The video client instance that was disconnected.
 * @param code The disconnection code indicating the reason for disconnection.
 * @param msg A message providing additional information about the disconnection.
 */
typedef void (API_CALL* on_disconnect_cb)(video_client ctx, int code, const char* msg);

/**
 * @brief Callback type for handling incoming data.
 * 
 * This callback function is called when video data is received from the video server.
 * 
 * @param ctx The video client instance that received the data.
 * @param data A pointer to the received video data.
 * @param size The size of the received video data.
 * @param frame_info Additional information about the video frame.
 */
typedef void (API_CALL* on_data_cb)(video_client ctx, uint8_t* data, size_t size, void* frame_info);


/**
 * @brief Initializes the video client API.
 * 
 * This function initializes the video client API and must be called before any other API functions.
 */
API_EXPORT void API_CALL api_init();

/**
 * @brief Creates a new video client instance.
 * 
 * This function initializes and returns a new video client instance that can be used to connect to a video server,
 * start video processing, and handle video data.
 * 
 * @return video_client A handle to the newly created video client instance.
 */
API_EXPORT video_client API_CALL create_video_client();

/**
 * @brief Releases a video client instance.
 * 
 * This function releases the resources associated with a video client instance.
 * 
 * @param ctx The video client instance to be released.
 */
API_EXPORT void API_CALL release_video_client(video_client ctx);

/**
 * @brief Connects a video client to a video server.
 * 
 * This function connects the specified video client instance to a video server using the provided URL.
 * 
 * @param ctx The video client instance to be connected.
 * @param url The URL of the video server to connect to.
  *           - tcp://192.168.0.1:{PORT}/{DEVICE}		(TCP/IP)
 *            - shdm://{DEVICE}   						(Shared Memory)
 * @param timeout_sec The timeout duration in seconds for the connection attempt.
 * @param cb The callback function to be called upon disconnection.
 * @return api_err_t The result of the connection attempt.
 */
API_EXPORT api_err_t API_CALL connect_video_client(video_client ctx, const char* url, float timeout_sec = 3, on_disconnect_cb cb = nullptr);

/**
 * @brief Disconnects a video client from the video server.
 * 
 * This function disconnects the specified video client instance from the video server.
 * 
 * @param ctx The video client instance to be disconnected.
 * @return api_err_t The result of the disconnection attempt.
 */
API_EXPORT api_err_t API_CALL disconnect_video_client(video_client ctx);

/**
 * @brief Starts video processing for a video client.
 * 
 * This function starts the video processing for the specified video client instance using the provided video processing context
 * and data callback function.
 * 
 * @param ctx The video client instance to start video processing for.
 * @param vp_ctx The video processing context containing settings such as GPU index, target format, and target FPS.
 * @param cb The callback function to be called with video data.
 * @return api_err_t The result of the start attempt.
 */
API_EXPORT api_err_t API_CALL start_video_client(video_client ctx, videoproc_context vp_ctx, on_data_cb cb);

/**
 * @brief Stops video processing for a video client.
 * 
 * This function stops the video processing for the specified video client instance.
 * 
 * @param ctx The video client instance to stop video processing for.
 * @return api_err_t The result of the stop attempt.
 */
API_EXPORT api_err_t API_CALL stop_video_client(video_client ctx);

/**
 * @brief Sets the maximum queue size for a video client.
 * 
 * This function sets the maximum number of items that can be held in the queue for the specified video client instance.
 * This queue is used to receive video data that needs to be processed from either TCP or shared memory.
 * Any data exceeding the queue size will be discarded.
 * 
 * @param ctx The video client instance for which to set the maximum queue size.
 * @param size The maximum number of items to be held in the queue. (default: 100)
 * @return api_err_t The result of the operation.
 */
API_EXPORT api_err_t API_CALL set_max_queue_size(video_client ctx, size_t size);


#ifdef __cplusplus
}
#endif

#endif // VIDEO_CLIENT_API_H