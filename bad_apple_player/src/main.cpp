#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <graphark/drawable_elements.h>
#include <graphark/program.h>

#include <generator>
#include <iostream>

void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint error_id,
                              GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam) {
  if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
    std::cerr << "OpenGL debug: " << message << "\n";
  }
}

void glfwErrorCallback(const int error, const char* description) {
  std::cerr << "GLFW error: " << description << "\n";
}

auto get_delta() -> double {
  double currentTime = glfwGetTime();
  static double lastTime = currentTime;
  double deltaTime = currentTime - lastTime;
  lastTime = currentTime;
  return deltaTime;
}

// Generate
auto read_coordinates() -> std::generator<std::tuple<float, float>> {
  std::ifstream coordinates_file("serialized_bad_apple.bin", std::ios::binary);
  if (!coordinates_file.is_open()) {
    std::cerr << "Could not open bin file\n";
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  const auto read_uint32 = [](std::ifstream& file) {
    uint32_t b;
    file.read(reinterpret_cast<char*>(&b), sizeof(uint32_t));
    return b;
  };
  uint32_t width = read_uint32(coordinates_file);        // 480
  uint32_t height = read_uint32(coordinates_file);       // 360
  uint32_t frame_count = read_uint32(coordinates_file);  // 100

  const auto read_uint8 = [](std::ifstream& file) {
    uint8_t b;
    file.read(reinterpret_cast<char*>(&b), sizeof(uint8_t));
    return b;
  };

  uint32_t current_width_pixel = 0;
  uint32_t current_height_pixel = 0;
  uint8_t byte;
  while (coordinates_file.read(reinterpret_cast<char*>(&byte), 1)) {
    for (int current_bit = 0; current_bit < 8; current_bit++) {
      const bool is_black = byte >> (7 - current_bit) & 0b1;
      if (is_black) {
        // Flip y
        co_yield {current_width_pixel, height - current_height_pixel};
      }
      current_width_pixel++;
      if (width <= current_width_pixel) {
        current_width_pixel = 0;
        current_height_pixel++;
      }
    }
  }
}

auto main() -> int {
  std::cout << "Hello World!" << std::endl;

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return 1;
  }

  glfwSetErrorCallback(glfwErrorCallback);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

  const auto window =
      // glfwCreateWindow(500, 500, "Bad Apple Graphark", nullptr, nullptr);
      glfwCreateWindow(480, 480, "Bad Apple Graphark", nullptr, nullptr);
  if (window == nullptr) {
    std::cerr << "Failed to create GLFW window." << std::endl;
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW." << std::endl;
    glfwTerminate();
    return 1;
  }

  // Enable debug
  []() {
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT != 0) {
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

      glDebugMessageCallback(glDebugCallback, nullptr);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                            nullptr, GL_TRUE);
    }
  }();

  const auto program =
      graphark::Program::Create("shaders/vertex.glsl", "shaders/fragment.glsl");
  if (!program) {
    std::cerr << "Failed to create the program: " << program.error().message
              << std::endl;
    glfwTerminate();
    return 1;
  }

  std::vector<float> vertices;
  vertices.reserve(480 * 480);
  for (const auto& [x, y] : read_coordinates()) {
    vertices.emplace_back(x);
    vertices.emplace_back(y);
  }

  glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

  // auto cam = Camera(-10, 10, -10, 10);
  // auto cam = Camera(0, 480, 0, 360);
  auto cam = Camera(0, 480, 0, 480);
  const auto handle_input = [&window, &cam](float delta_time) {
    const auto is_pressed = [&window](int key) {
      return glfwGetKey(window, key) == GLFW_PRESS;
    };

    float pan_speed = (cam.maxX() - cam.minX()) * 0.5f;
    float zoom_factor = 2.5f;

    if (is_pressed(GLFW_KEY_LEFT)) {
      cam.pan(-pan_speed * delta_time, 0.0f);
    }
    if (is_pressed(GLFW_KEY_RIGHT)) {
      cam.pan(pan_speed * delta_time, 0.0f);
    }
    if (is_pressed(GLFW_KEY_UP)) {
      cam.pan(0.0f, pan_speed * delta_time);
    }
    if (is_pressed(GLFW_KEY_DOWN)) {
      cam.pan(0.0f, -pan_speed * delta_time);
    }
    if (is_pressed(GLFW_KEY_EQUAL)) {
      cam.zoom(static_cast<float>(
          std::pow(1.0 / static_cast<double>(zoom_factor), delta_time)));
    }
    if (is_pressed(GLFW_KEY_MINUS)) {
      cam.zoom(std::pow(zoom_factor, delta_time));
    }
  };

  glClearColor(0.0, 0.0, 0.0, 0.0);
  while (!glfwWindowShouldClose(window)) {
    const auto delta = get_delta();
    const auto m_projection =
        // glm::ortho(0.0F, 480.0F, 0.0F, 360.0F, 1.0F, 0.0F);
    glm::ortho(cam.minX(), cam.maxX(), cam.minY(), cam.maxY(), -1.0f, 1.0f);
    const auto grid = graphark::elements::get_grid_drawable(cam);
    const graphark::Drawable2D axis =
        graphark::elements::get_axis_drawable(cam);

    const graphark::Drawable2D bad_apple_frame =
        graphark::Drawable2D(vertices, GL_POINTS);

    handle_input(delta);

    program->Use();
    glClear(GL_COLOR_BUFFER_BIT);

    program->SetUniformMatrix("mProjection", m_projection)
        .or_else(print_err_and_abort_execution<void>);

    program->SetUniformVector("vColor", glm::vec4(0.5, 0.5, 0.5, 1.0))
        .or_else(print_err_and_abort_execution<void>);
    grid.Draw();

    program->SetUniformVector("vColor", glm::vec4(1.0, 1.0, 1.0, 1.0))
        .or_else(print_err_and_abort_execution<void>);
    axis.Draw();

    program->SetUniformVector("vColor", glm::vec4(1.0, 0.5, 0.5, 1.0))
        .or_else(print_err_and_abort_execution<void>);
    bad_apple_frame.Draw();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}