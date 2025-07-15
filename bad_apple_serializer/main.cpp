#include <expected>
#include <fstream>
#include <generator>
#include <iostream>
#include <map>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <ranges>

using namespace cv;

auto single_frame_main() {
  const auto get_test_frame = []() -> std::tuple<int, cv::Mat> {
    auto video_capture = cv::VideoCapture("bad_apple.mp4");
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

  cv::Mat grayscale_frame;
  if (frame.channels() > 1) {
    cv::cvtColor(frame, grayscale_frame, cv::COLOR_BGR2GRAY);
  } else {
    grayscale_frame = frame.clone();
  }
  cv::Mat binary_frame;
  cv::threshold(grayscale_frame, binary_frame, 128, 1, cv::THRESH_BINARY_INV);

  std::vector<bool> binaryData;
  binaryData.reserve(binary_frame.total());

  cv::MatConstIterator_<uchar> it = binary_frame.begin<uchar>(),
                               end = binary_frame.end<uchar>();
  for (; it != end; ++it) {
    binaryData.push_back(*it != 0);
  }

  const auto bytes_generator = [&binaryData]() -> std::generator<uint8_t> {
    uint8_t current_byte = 0;
    uint8_t bit_count = 0;
    for (auto is_pixel_black : binaryData) {
      if (is_pixel_black) {
        current_byte |= 0b1 << (7 - bit_count);
      }
      bit_count++;
      if (bit_count == 8) {
        co_yield current_byte;
        current_byte = 0;
        bit_count = 0;
      }
    }
    if (bit_count != 0) {
      co_yield current_byte;
      current_byte = 0;
      bit_count = 0;
    }
  };

  std::ofstream serialized_file("serialized_bad_apple.bin");
  if (!serialized_file.is_open()) {
    std::cerr << "Could not open output file" << std::endl;
    return 1;
  }

  const auto write_uint8 = [](std::ofstream& file, const uint8_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  };
  const auto write_uint32 = [](std::ofstream& file, const uint32_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  };

  const uint32_t width = frame.cols;
  const uint32_t height = frame.rows;
  constexpr uint32_t frame_count = 100;
  write_uint32(serialized_file, width);
  write_uint32(serialized_file, height);
  write_uint32(serialized_file, frame_count);

  for (const auto byte : bytes_generator()) {
    write_uint8(serialized_file, byte);
  }
  return 0;
}

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
  // return single_frame_main();

  auto get_frames_generator =
      [](cv::VideoCapture& video_capture) -> std::generator<cv::Mat> {
    size_t frame_number = 0;
    cv::Mat frame;
    while (video_capture.isOpened()) {
      video_capture.read(frame);
      if (frame.empty()) {
        std::cout << "End of video stream" << std::endl;
        break;
      }
      co_yield frame;
    }
  };
  auto video_capture = cv::VideoCapture("bad_apple.mp4");
  if (!video_capture.isOpened()) {
    // return std::unexpected("Could not open video capture");
    std::cerr << "Could not open video capture" << std::endl;
    return 1;
  }

  std::ofstream serialized_file("serialized_bad_apple.bin", std::ios::binary);
  if (!serialized_file.is_open()) {
    std::cerr << "Could not open output file" << std::endl;
    return 1;
  }

  const auto write_uint8 = [](std::ofstream& file, const uint8_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  };
  const auto write_uint32 = [](std::ofstream& file, const uint32_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
  };

  std::ofstream bad_apple_ascii_file("bad_apple_ascii.txt");

  bool first_frame = true;
  for (const auto& frame : get_frames_generator(video_capture)) {
    if (first_frame) {
      const uint32_t width = frame.cols;
      const uint32_t height = frame.rows;
      constexpr uint32_t frame_count = 100;
      write_uint32(serialized_file, width);
      write_uint32(serialized_file, height);
      write_uint32(serialized_file, frame_count);
      first_frame = false;
    }

    cv::Mat grayscale_frame;
    if (frame.channels() > 1) {
      cv::cvtColor(frame, grayscale_frame, cv::COLOR_BGR2GRAY);
    } else {
      grayscale_frame = frame.clone();
    }
    cv::Mat binary_frame;
    cv::threshold(grayscale_frame, binary_frame, 128, 1, cv::THRESH_BINARY_INV);

    const auto bytes_generator =
        [&binary_frame, &bad_apple_ascii_file]() -> std::generator<uint8_t> {
      uint8_t current_byte = 0;
      uint8_t bit_count = 0;

      for (int row = 0; row < binary_frame.rows; ++row) {
        for (int col = 0; col < binary_frame.cols; ++col) {
          const bool is_pixel_black = binary_frame.at<uint8_t>(row, col);

          if (is_pixel_black) {
            current_byte |= (1 << (7 - bit_count));
            bad_apple_ascii_file << "@";
          } else {
            bad_apple_ascii_file << "-";
          }

          bit_count++;
          if (bit_count == 8) {
            co_yield current_byte;
            current_byte = 0;
            bit_count = 0;
          }
        }
        bad_apple_ascii_file << "\n";  // newline after each row
      }

      if (bit_count > 0) {
        co_yield current_byte;
      }
    };

    for (const auto byte : bytes_generator()) {
      write_uint8(serialized_file, byte);
    }
  }

  return 0;
}
