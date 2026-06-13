#include <cmath>
#include <vector>
#include <algorithm>
#include "myUI.h"

constexpr float C = 50.0f;
constexpr float G = 4.0f;
constexpr float PI = 3.14159265358979323846f;

struct Vec {
    float x;
    float y;

    Vec (float x, float y) : x(x), y(y) {}

    Vec operator+(const Vec& other) const {
        return {x + other.x, y + other.y};
    }

    Vec operator-(const Vec& other) const {
        return {x - other.x, y - other.y};
    }

    Vec operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }

    Vec operator/(float scalar) const {
        return {x / scalar, y / scalar};
    }

    Vec& operator+=(const Vec& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec& operator-=(const Vec& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vec& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    float get_length() const {
        return sqrt(x * x + y * y);
    }

    Vec get_normalized() const {
        const float length = get_length();
        if (length == 0.0f) return {0.0f, 0.0f};
        return {x / length, y / length};
    }
};

struct State {
    float r;
    float phi;
    float v_r;
};

struct RayTrailObj {
    float x;
    float y;
    float alpha = 1;

    RayTrailObj(Vec pos) {
        x = pos.x;
        y = pos.y;
    }

    void update(float dt) {
        alpha = std::max(0.0f, alpha - dt * 0.1f);
    }
};

struct Mesh {
    GLuint vao;
    GLuint vbo;
    GLsizei vertexCount;
};

struct BlackHole {
    Vec pos;
    float r_s;  // Schwarzschild radius
    float mass;

    BlackHole(float x, float y, float mass, float r_s) : pos(x, y), mass(mass), r_s(r_s) {}
};

struct Ray {
    float r;  // Dist. to black hole
    float phi;  // Angle to black hole
    float v_r;  // Radial velocity
    float L;  // Angular momentum (conserved)

    std::vector<RayTrailObj> trail;
    GLuint vao;  // Vertex Array Object
    GLuint vbo;  // Vertex Buffer Object
    Mesh mesh {0, 0, 0};

    void updateMesh() {
        // Setting the object data
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, trail.size() * sizeof(RayTrailObj), trail.data(), GL_STATIC_DRAW);

        // x, y
        glVertexAttribPointer(
            0, // index
            2, // size
            GL_FLOAT, // type
            GL_FALSE, // normalized
            3 * sizeof(float), // stride
            (void*) (0 * sizeof(float)) // offset
        );
        glEnableVertexAttribArray(0);

        // alpha
        glVertexAttribPointer(
            1, // index
            1, // size
            GL_FLOAT, // type
            GL_FALSE, // normalized
            3 * sizeof(float), // stride
            (void*) (2 * sizeof(float)) // offset
        );
        glEnableVertexAttribArray(1);

        mesh = {vao, vbo, (int) trail.size()};
    }

    Ray(float x, float y, float angle, BlackHole& blackHole) {
        Vec rel = Vec(x, y) - blackHole.pos;
        r = rel.get_length();
        phi = atan2(rel.y, rel.x);
        trail.emplace_back(Vec(x, y));

        Vec e_r = Vec(cos(phi), sin(phi));  // Points away from black hole
        Vec e_phi = Vec(-sin(phi), cos(phi));  // Points tangentially
        Vec v0 = Vec(cos(angle), sin(angle)) * C;  // Initial velocity
        v_r = v0.x * e_r.x + v0.y * e_r.y;  // Tangential velocity
        float v_phi = v0.x * e_phi.x + v0.y * e_phi.y;  // Angular velocity
        L = r * v_phi;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    void move(float dt, BlackHole& blackHole) {
        /*
        const Vec diff = blackHole.pos - pos;
        const float r = diff.get_length();

        if (r <= blackHole.r_s) return;

        const Vec dir = diff.get_normalized();
        const Vec velDir = vel.get_normalized();

        const float strength = G * blackHole.mass / (r * r);
        Vec gravity = dir * strength;

        const float radial = gravity.x * velDir.x + gravity.y * velDir.y;
        Vec bend = gravity - velDir * radial;

        vel += bend * dt;
        vel = vel.get_normalized() * C;

        pos += vel * dt;
        */
        if (r <= blackHole.r_s) return;  // Beyond event horizon, absorbed
        float M = blackHole.r_s * 0.5f;  // Mass of black hole

        // Here v_r is dr/dλ tangentially. [NOTE: dλ is minute step of light (not time-step)]
        float dphi = L / (r * r);  // Angular velocity
        float dv_r = (L * L) / (r * r * r) - (3.0f * M * L * L) / (r * r * r * r);  // Tangential acceleration

        // Euler integration
        r += v_r * dt;
        phi += dphi * dt;
        v_r += dv_r * dt;
    }

    void update(float dt, BlackHole& blackHole) {
        trail.emplace_back(blackHole.pos + Vec(cos(phi), sin(phi)) * r);

        for (RayTrailObj& rayTrailObj: trail) {
            rayTrailObj.update(dt);
        }

        trail.erase(
            std::remove_if(
                trail.begin(),
                trail.end(),
                [](const RayTrailObj& p) {
                    return p.alpha <= 0.0f;
                }
            ),
            trail.end()
        );

        updateMesh();
    }

    void draw() {
        if (mesh.vertexCount > 0) {
            glBindVertexArray(mesh.vao);
            glDrawArrays(GL_LINE_STRIP, 0, mesh.vertexCount);
        }
    }

    ~Ray() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }
};

struct Beam {
    std::vector<Ray> rays;

    Vec startPos;
    Vec endPos;
    float startAngle;
    float endAngle;
    int rayCount;

    Beam() = default;
    Beam(Vec startPos, Vec endPos, float startAngle, float endAngle, int rayCount, BlackHole& blackHole)
    : startPos(startPos), endPos(endPos), startAngle(startAngle), endAngle(endAngle), rayCount(rayCount) {

        rays.clear();
        rays.reserve(rayCount);

        for (int idx = 0; idx < rayCount; idx++) {
            float step = (rayCount == 1) ? 0.0f : (float) idx / (rayCount - 1);
            float angle = startAngle * (1.0f - step) + endAngle * step;
            Vec pos = startPos * (1.0f - step) + endPos * step;
            Vec dir = Vec(cos(angle), sin(angle)).get_normalized();
            Vec vel = dir * C;
            rays.emplace_back(pos.x, pos.y, angle, blackHole);
        }
    }

    void move(float dt, BlackHole& blackHole) {
        for (Ray& ray: rays) {
            if (ray.r > blackHole.r_s) {
                ray.move(dt, blackHole);
            }
        }
    }

    void update(float dt, BlackHole& blackHole) {
        for (Ray& ray: rays) {
            ray.update(dt, blackHole);
        }
    }

    void draw() {
        for (Ray& ray: rays) {
            ray.draw();
        }
    }
};

Mesh getCircle(int x, int y, int radius, int resolution = 360) {
    std::vector<float> vertices(resolution * 2);

    for (int step = 0; step < resolution; step++) {
        float angle = step * 2 * PI / resolution;
        vertices[step * 2] = x + radius * std::cos(angle);
        vertices[step * 2 + 1] = y + radius * std::sin(angle);
    }

    GLuint vao;  // Vertex Array Object
    GLuint vbo;  // Vertex Buffer Object

    // Init objects into variables
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Setting the object data
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // We have only 1 attribute, the position, which is 2 floats (x and y)
    glVertexAttribPointer(
        0, // index
        2, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized
        2 * sizeof(float), // stride
        nullptr // offset
    );
    glEnableVertexAttribArray(0);

    return Mesh {vao, vbo, resolution};
}

int main() {
    if (myUI::init() != 0) return -1;

    GLFWwindow* window = myUI::createWindow(1280, 720, "OpenGL");
    if (!window) return -1;

    GLuint black_hole_shaders = myUI::createShaderProgram("shaders/black_hole/vertex.glsl", "shaders/black_hole/fragment.glsl");
    GLuint ray_shaders = myUI::createShaderProgram("shaders/ray/vertex.glsl", "shaders/ray/fragment.glsl");

    BlackHole blackHole(640, 360, 25000.0f, 50.0f);
    Mesh circleMesh = getCircle(blackHole.pos.x, blackHole.pos.y, blackHole.r_s);
    Mouse mouse;

    Beam beam(Vec(100, 0), Vec(100, 720), -0.6f, 0.6f, 180, blackHole);

    float timer = 0;
    float lastTime = glfwGetTime();
    const int numSubframes = 20;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        timer += dt;
        if (timer >= 2) {
            timer = 0;
            std::cout << "FPS: " << 1 / dt << "\n";
        }

        glfwPollEvents();
        myUI::getMouseData(window, mouse);

        const float subDt = dt / numSubframes;
        for (int subframeIdx = 0; subframeIdx < numSubframes; subframeIdx++) {
            beam.move(subDt, blackHole);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(black_hole_shaders);
        glUniform2f(glGetUniformLocation(black_hole_shaders, "screenSize"), 1280.0f, 720.0f);
        glUniform3f(glGetUniformLocation(black_hole_shaders, "color"), 1.0f, 1.0f, 1.0f);

        glBindVertexArray(circleMesh.vao);
        glDrawArrays(GL_LINE_LOOP, 0, circleMesh.vertexCount);

        glUseProgram(ray_shaders);
        glUniform2f(glGetUniformLocation(ray_shaders, "screenSize"), 1280.0f, 720.0f);
        glUniform3f(glGetUniformLocation(ray_shaders, "color"), 1.0f, 1.0f, 1.0f);

        beam.update(dt, blackHole);
        beam.draw();

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &circleMesh.vao);
    glDeleteBuffers(1, &circleMesh.vbo);

    glDeleteProgram(black_hole_shaders);
    glDeleteProgram(ray_shaders);

    myUI::close(window);
    return 0;
}
