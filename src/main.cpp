#include <cmath>
#include <vector>
#include <algorithm>
#include "myUI.h"

constexpr float C = 75.0f;
constexpr float G = 4.0f;
constexpr float PI = 3.14159265358979323846f;

constexpr float SCREEN_WIDTH = 1200;
constexpr float SCREEN_HEIGHT = 800;

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

struct State {
    float r;  // Distance to black hole
    float phi;  // Angle to black hole
    float v_r;  // Radial velocity

    State operator+(const State& other) const {
        return {r + other.r, phi + other.phi, v_r + other.v_r};
    }

    State operator-(const State& other) const {
        return {r - other.r, phi - other.phi, v_r - other.v_r};
    }

    State operator*(float c) const {
        return {r * c, phi * c, v_r * c};
    }

    State operator/(float c) const {
        return {r / c, phi / c, v_r / c};
    }

    State& operator+=(const State& other) {
        r += other.r;
        phi += other.phi;
        v_r += other.v_r;
        return *this;
    }

    State& operator-=(const State& other) {
        r -= other.r;
        phi -= other.phi;
        v_r -= other.v_r;
        return *this;
    }

    State& operator*=(float c) {
        r *= c;
        phi *= c;
        v_r *= c;
        return *this;
    }

    State& operator/=(float c) {
        r /= c;
        phi /= c;
        v_r /= c;
        return *this;
    }
};

State derivatives_of(State& state, float L, float M) {
    State dervs;
    dervs.r = state.v_r;
    dervs.phi = L / pow(state.r, 2);  // Angular velocity

    // Here v_r is dr/dλ tangentially. [NOTE: dλ is minute step of light (not time-step)]
    dervs.v_r = pow(L, 2) / pow(state.r, 3) - (3.0f * M * pow(L, 2)) / pow(state.r, 4);  // Tangential acceleration
    return dervs;
}

struct Ray {
    State state;
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
        state.r = rel.get_length();
        state.phi = atan2(rel.y, rel.x);
        trail.emplace_back(Vec(x, y));

        Vec e_r = Vec(cos(state.phi), sin(state.phi));  // Points away from black hole
        Vec e_phi = Vec(-sin(state.phi), cos(state.phi));  // Points tangentially
        Vec v0 = Vec(cos(angle), sin(angle)) * C;  // Initial velocity
        state.v_r = v0.x * e_r.x + v0.y * e_r.y;  // Tangential velocity
        float v_phi = v0.x * e_phi.x + v0.y * e_phi.y;  // Angular velocity
        L = state.r * v_phi;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    void move(float dt, BlackHole& blackHole) {
        /*
        // Newtonian mechanics
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
        if (state.r <= blackHole.r_s) return;  // Beyond event horizon, absorbed
        float M = blackHole.r_s * 0.5f;  // Mass of black hole

        // RK4
        State temp;
        const State k1 = derivatives_of(state, L, M);
        temp = state + k1 * dt * 0.5;
        const State k2 = derivatives_of(temp, L, M);
        temp = state + k2 * dt * 0.5;
        const State k3 = derivatives_of(temp, L, M);
        temp = state + k3 * dt;
        const State k4 = derivatives_of(temp, L, M);

        const State avg_dervs = (k1 + k2 * 2 + k3 * 2 + k4) / 6;
        state += avg_dervs * dt;
        
        /*
        // Euler integration
        State derivs = derivatives_of(state, L, M);
        state += derivs * dt;
        */
    }

    void update(float dt, BlackHole& blackHole) {
        trail.emplace_back(blackHole.pos + Vec(cos(state.phi), sin(state.phi)) * state.r);

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
            if (ray.state.r > blackHole.r_s) {
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

    GLFWwindow* window = myUI::createWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL");
    if (!window) return -1;

    GLuint black_hole_shaders = myUI::createShaderProgram("shaders/black_hole/vertex.glsl", "shaders/black_hole/fragment.glsl");
    GLuint ray_shaders = myUI::createShaderProgram("shaders/ray/vertex.glsl", "shaders/ray/fragment.glsl");

    BlackHole blackHole(600, 400, 25000.0f, 35.0f);
    Beam beam(Vec(100, 250), Vec(100, 550), -0.2f, 0.2f, 1500, blackHole);
    Mesh circleMesh = getCircle(blackHole.pos.x, blackHole.pos.y, blackHole.r_s);
    Mouse mouse;

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
        glUniform2f(glGetUniformLocation(black_hole_shaders, "screenSize"), SCREEN_WIDTH, SCREEN_HEIGHT);
        glUniform3f(glGetUniformLocation(black_hole_shaders, "color"), 1.0f, 1.0f, 1.0f);

        glBindVertexArray(circleMesh.vao);
        glDrawArrays(GL_LINE_LOOP, 0, circleMesh.vertexCount);

        glUseProgram(ray_shaders);
        glUniform2f(glGetUniformLocation(ray_shaders, "screenSize"), SCREEN_WIDTH, SCREEN_HEIGHT);
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
