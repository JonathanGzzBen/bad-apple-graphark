#include <fstream>
#include <generator>
#include <iostream>
#include <map>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace cv;

int main() {
  /*
   *
   * Second Algorithm Idea
   *
   * Frame is read and each pixel is stored as a single bit which is 1 if it is
   * above a specified threshold of grayscale, 0 otherwise.
   *
   * In the output file there will be a header which will specify the width x
   * height of the frames
   *
   * Then it will be read from binary file byte per byte, and each byte will be
   * unpackaged and the line width will be considered, as the last byte of a
   * line might have padding 0s.
   *
   * Once the frame has been loaded for example into an array of bytes, the
   * width and height in the header of the file will be considered to create a
   * linear map function which will map each byte to a range which will conform
   * to the orthogonal view from Graphark.
   *
   */

  const auto get_test_frame = []() -> std::tuple<int, cv::Mat> {
    auto video_capture = cv::VideoCapture("bad_apple.mp4", IMREAD_GRAYSCALE);
    if (!video_capture.isOpened()) {
      std::cout << "Could not open video capture" << std::endl;
      return {1, cv::Mat{}};
    }
    cv::Mat frame;
    size_t frame_number = 0;
    while (video_capture.isOpened()) {
      video_capture.read(frame);
      if (frame.empty()) {
        std::cout << "End of video stream" << std::endl;
        break;
      }
      frame_number++;

      if (frame_number == 180) {  // Reimu frame, mostly white
        // if (frame_number == 540) { // Marisa frame, mostly black
        return {0, frame};
      }
    }
    return {1, cv::Mat{}};
  };

  auto [res, frame] = get_test_frame();
  if (res != 0) {
    std::cout << "Could not read the frame: " << res << std::endl;
    return 1;
  }
  // imshow("frame180", frame);
  // cv::waitKey(0);
  imwrite("test_bad_apple_frame.jpg", frame);

  using Grayscale = uint8_t;

  const auto bytes_generator = [&frame]() -> std::generator<uint8_t> {
    for (int row = 0; row < frame.rows; row++) {
      unsigned short current_byte = 0;
      uint8_t bit_count = 0;
      for (int col = 0; col < frame.cols; col++) {
        const auto color = frame.at<cv::Vec3b>(row, col);
        const auto pixel_grayscale = static_cast<Grayscale>(color[0]);
        const auto pixel_value = 127 < pixel_grayscale;

        current_byte |= pixel_value << bit_count;

        bit_count++;
        if (bit_count == 8) {
          co_yield current_byte;
          bit_count = 0;
          current_byte = 0;
        }
      }
      if (bit_count == 0) {
        current_byte <<= 8 - bit_count;  // Pad with zeros to the right
        co_yield current_byte;
      }
    }
  };

  auto output_file =
      std::ofstream("serialized_bad_apple.bin", std::ios::binary);

  if (!output_file.is_open()) {
    std::cerr << "Could not open output file" << std::endl;
    return 1;
  }

  const auto write_uint8 = [](std::ofstream& file, const uint8_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  };
  const auto write_uint32 = [](std::ofstream& file, const uint32_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  };

  // One byte is not enough for storing width or height
  const uint32_t width = frame.cols;
  const uint32_t height = frame.rows;
  constexpr uint32_t frame_count = 100;
  write_uint32(output_file, width);
  write_uint32(output_file, height);
  write_uint32(output_file, frame_count);

  size_t bytes_written = 0;
  for (const auto byte : bytes_generator()) {
    write_uint8(output_file, byte);
    bytes_written++;
  }
  output_file.flush();
  output_file.close();
  std::cout << std::format("{} bytes written\n", bytes_written);

  static constexpr auto target_fps = 30;
  static constexpr auto total_seconds = 315;
  const auto bytes_per_frame = bytes_written;
  const auto total_bytes_per_video =
      target_fps * total_seconds * bytes_per_frame;

  std::cout << std::format("Total bytes required: {}", total_bytes_per_video)
            << std::endl;

  return 0;
}
