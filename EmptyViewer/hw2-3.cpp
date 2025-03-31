#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
int Width = 512;
int Height = 512;
int inf = 1'000'000;
std::vector<float> OutputImage;
// -------------------------------------------------

//두 점사이의 거리를 계산하는 함수
float dist(vec3 a, vec3 b) {
	return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
}

//선
class Ray {
public:
	//pos를 지나는 3차원 선
	//pos.x + t*dir.x , pos.y + t*dir.y, pos.z + t*dir.z 를 지난다.
	vec3 pos;
	vec3 dir;

	Ray(vec3 position, vec3 direction) {
		pos = position;
		dir = direction;
	}
};

//카메라
class Camera {
public:
	//카메라의 위치
	vec3 pos;

	//스크린 방향, u,v,w
	vec3 u, v, w;

	float l, r, b, t, d;
	//dir방향으로 d만큼 떨어진 곳에, 방향이 u,v,w인 [l,r] X [b,t] 스크린이 존재함을 의미

	Camera(vec3 position, vec3 U, vec3 V, vec3 W, float L, float R, float B, float T, float D) {
		u = U;
		v = V;
		w = W;
		pos = position;
		l = L;
		r = R;
		b = B;
		t = T;
		d = D;
	}

	//해상도가 nx, ny일 때, ix, iy칸에 해당하는 선(Ray)를 반환한다.
	Ray getRay(int ix, int iy, int nx, int ny) {
		ix++, iy++;

		//랜덤한 실수값을 더해준다. [-0.5, 0.5)사이의 실수이다.
		float u_random = (float)(rand() % 10000) / 10000.0f - 0.5f;
		float v_random = (float)(rand() % 10000) / 10000.0f - 0.5f; 

		float u_pixel = l + (r - l) * (ix + 0.5f + u_random) / nx;
		float v_pixel = b + (t - b) * (iy + 0.5f + v_random) / ny;

		vec3 s = pos + (u_pixel * u) + (v_pixel * v) - (d * w);
		vec3 p = pos;
		vec3 d = s - pos;
		Ray* ray = new Ray(p, d);
		return *ray;
	}
};

//광원
class Light {
public:
	//빛의 위치와 빛의 강도
	vec3 pos;
	float light;

	Light(vec3 Pos, float Light) {
		pos = Pos;
		light = Light;
	}
};

class Surface {
public:
	//override를 위한 함수
	virtual float intersect(Ray ray, float tMin, float tMax) {
		return tMax + 1;
	}
	virtual vec3 getNormal(vec3 point) {
		return { 0,0,0 };
	}
	virtual vec3 shade(Ray ray, vec3 point, vec3 normal, Light light, bool isBlock) {
		return { 0,0,0 };
	}
};

//평면
class Plane : public Surface {
public:
	//평면의 y좌표
	float y = 0;
	float p = 0;
	vec3 ka, kd, ks;

	Plane(float ycor, vec3 KA, vec3 KD, vec3 KS, float P) {
		y = ycor;
		ka = KA;
		kd = KD;
		ks = KS;
		p = P;
	}

	vec3 getNormal(vec3 point) {
		//수직 선을 리턴하면 된다.
		return { 0,1,0 };
	}

	vec3 shade(Ray ray, vec3 point, vec3 normal, Light light, bool isBlock) {
		vec3 l = light.pos - point;
		l = normalize(l);
		vec3 v = -ray.dir;
		v = normalize(v);
		vec3 h = normalize(v + l);
		//l은 point에서 광원으로의 방향
		//v는 카메라로의 방향
		//h는 l와 v의 중간 방향을 의미한다. 

		vec3 resLight = ka * 1.0f;
		Ray* rayToLight = new Ray(point, normalize(light.pos - point));
		if (isBlock) { //가로막히지 않았다면 빛으로 생기는 밝기값을 더해준다.
			resLight += kd * light.light * max(0.0f, dot(normal, l)) + ks * pow(max(0.0f, dot(normal, h)), p);
		}
		return resLight;
	}

	//평면과 선이 만나는 지점을 반환한다.
	float intersect(Ray ray, float tMin, float tMax) override {
		//평면과 선이 만나는 지점 리턴하기, 여러개라면 ray.pos에 가까운 것을 리턴

		if (ray.dir.y == 0) {
			if (y == ray.pos.y) {
				//항상 만난다
				return tMin;
			}
			else {
				//항상 만나지 않는다
				return tMax + 1;
			}
		}
		float rest = (y - ray.pos.y) / (ray.dir.y);
		if (tMin <= rest && rest <= tMax) return rest;
		else return tMax + 1;
	}
};

//구
class Sphere : public Surface {
public:
	//구의 중심 좌표와 반지름
	vec3 pos;
	vec3 ka, kd, ks;
	float r = 0;
	float p = 0;

	Sphere(vec3 position, float radius, vec3 KA, vec3 KD, vec3 KS, float P) {
		pos = position;
		r = radius;
		ka = KA;
		kd = KD;
		ks = KS;
		p = P;
	}

	vec3 getNormal(vec3 point) {
		//수직 선을 리턴하면 된다.
		//point와 pos를 통해 계산하면 된다.
		vec3 res = point - pos;
		return normalize(res);
	}

	vec3 shade(Ray ray, vec3 point, vec3 normal, Light light, bool isBlock) {
		vec3 l = light.pos - point;
		l = normalize(l);
		vec3 v = -ray.dir;
		v = normalize(v);
		vec3 h = normalize(v + l);
		//l은 point에서 광원으로의 방향
		//v는 카메라로의 방향
		//h는 l와 v의 중간 방향을 의미한다. 

		vec3 resLight = ka * 1.0f;
		Ray* rayToLight = new Ray(point, normalize(light.pos - point));
		if (isBlock) { //가로막히지 않았다면 빛으로 생기는 밝기값을 더해준다.
			resLight += kd * light.light * max(0.0f, dot(normal, l)) + ks * pow(max(0.0f, dot(normal, h)), p);
		}
		return resLight;
	}

	//구와 ray가 만나는 지점을 반환한다.
	float intersect(Ray ray, float tMin, float tMax) override {
		//원과 선이 만나는 지점 리턴하기, 여러개라면 ray.pos에 가까운 것을 리턴
		vec3 diff = ray.pos - pos;
		float a = dot(ray.dir, ray.dir);
		float b = 2 * dot(diff, ray.dir);
		float c = dot(diff, diff) - r * r;
		//a*t^2 + b*t + c = 0 를 만족하는 t가 구하고 싶은 값이다.

		if (b * b - 4 * a * c < 0) { //ray와 원이 만나지 않을 때 (허근)
			return tMax + 1;
		}

		float t1 = (-b - std::sqrt(b * b - 4 * a * c)) / (2 * a);
		float t2 = (-b + std::sqrt(b * b - 4 * a * c)) / (2 * a);
		//가능한 t를 리턴, 없다면 tMax+1(불가능한 값의 표현), 여러 개라면 최솟값을 반환한다.
		int sign1 = 0, sign2 = 0;
		if (tMin <= t1 && t1 <= tMax) sign1 = 1;
		if (tMin <= t2 && t2 <= tMax) sign2 = 1;
		if (sign1 == 0 && sign2 == 1) return t2;
		else if (sign1 == 1 && sign2 == 0) return t1;
		else if (sign1 == 1 && sign2 == 1) return min(t1, t2);
		else return tMax + 1;
	}
};


//물체들의 집합
class Scene {
public:
	std::vector<Surface*> v;

	void add(Surface* s) {
		v.push_back(s);
	}

	//선과 물체가 만나는 부분과, 만나는 물체를 반환한다 
	Surface* hit(Ray ray, float tMin, float tMax, float* t) {
		float rest = tMax + 1;
		Surface* ret = NULL;
		for (Surface* i : v) {
			//i와 누가 만나는지를 계산
			//i와의 intersect 계산하기
			float curt = i->intersect(ray, tMin, tMax);
			if (tMin <= curt && curt <= tMax && curt < rest) {
				rest = curt;
				ret = i;
			}
		}
		//어디서 만나는지 반환
		*t = rest;
		return ret;
	}

	//ray와 물체가 만나는지를 확인한다.
	bool checkBlock(Ray ray, float tMin, float tMax) {
		for (Surface* i : v) { //intersect 계산하기
			float curt = i->intersect(ray, tMin, tMax);
			if (tMin <= curt && curt <= tMax) return false;
		}
		return true;
	}

	//ray와 물체가 만나는 지 확인하고, 물체의 밝기값을 리턴한다.
	vec3 trace(Ray ray, float tMin, float tMax, Light light) {
		//ray와 Surface들이 만나는 지점을 리턴하기, tMin과 tMax사이의 범위만 탐색한다.
		float t = 0;

		//어떤 물체와 닿는지 계산한다.
		Surface* surface = hit(ray, tMin, tMax, &t);

		if (surface != NULL) {
			//해당 surface의 밝기값을 계산한다.
			vec3 point = ray.pos + ray.dir * t;
			//point에서 광원으로의 방향
			vec3 normal = surface->getNormal(point);
			//빛으로 가는 선을 계산한다.
			Ray* rayToLight = new Ray(point, normalize(light.pos - point));
			//광원과 닿는 부분이 있는지 계산한다.
			bool isBlock = checkBlock(*rayToLight, 0.1, inf);
			//물체의 밝기를 계산한다. 
			return surface->shade(ray, point, normal, light, isBlock);
		}
		else return { 0,0,0 };
	}
};

void render()
{
	//Create our image. We don't want to do this in 
	//the main loop since this may be too slow and we 
	//want a responsive display of our beautiful image.
	//Instead we draw to another buffer and copy this to the 
	//framebuffer using glDrawPixels(...) every refresh
	OutputImage.clear();

	//물체 생성
	Surface* s1 = new Sphere({ -4.0,0.0,-7.0 }, 1.0, { 0.2,0,0 }, { 1,0,0 }, { 0,0,0 }, 0);
	Surface* s2 = new Sphere({ 0.0,0.0,-7.0 }, 2.0, { 0,0.2,0 }, { 0,0.5,0 }, { 0.5,0.5,0.5 }, 32);
	Surface* s3 = new Sphere({ 4.0,0.0,-7.0 }, 1.0, { 0,0,0.2 }, { 0,0,1 }, { 0,0,0 }, 0);
	Surface* p1 = new Plane(-2.0, { 0.2,0.2,0.2 }, { 1,1,1 }, { 0,0,0 }, 0);
	Light* light = new Light({ -4,4,-3 }, 1);

	//Scene에 넣기
	Scene scene;
	scene.add(s1);
	scene.add(s2);
	scene.add(s3);
	scene.add(p1);

	//카메라 생성
	Camera* camera = new Camera({ 0,0,0 }, { 1,0,0 }, { 0,1,0 }, { 0,0,1 }, -0.1, 0.1, -0.1, 0.1, 0.1);

	for (int j = 0; j < Height; ++j)
	{
		for (int i = 0; i < Width; ++i)
		{
			//randomNum번만큼 랜덤하게 계산한다.
			int randomNum = 64;
			vec3 color = { 0,0, 0 };
			for (int p = 0; p < randomNum; p++) {
				//x,y에 해당하는 선
				Ray ray = camera->getRay(i, j, Width, Height);

				//ray와 물체가 만난다면 1, 만나지 않는다면 0으로 나타낸다.
				vec3 curColor = scene.trace(ray, 0.01, inf, *light);
				color += curColor;
			}
			color /= randomNum;

			//gamma correction 계산하기
			float r = 2.2;
			color.x = pow(color.x, 1 / r);
			color.y = pow(color.y, 1 / r);
			color.z = pow(color.z, 1 / r);

			// set the color
			OutputImage.push_back(color.x); // R
			OutputImage.push_back(color.y); // G
			OutputImage.push_back(color.z); // B
		}
	}
}


void resize_callback(GLFWwindow*, int nw, int nh)
{
	//This is called in response to the window resizing.
	//The new width and height are passed in so we make 
	//any necessary changes:
	Width = nw;
	Height = nh;
	//Tell the viewport to use all of our screen estate
	glViewport(0, 0, nw, nh);

	//This is not necessary, we're just working in 2d so
	//why not let our spaces reflect it?
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, static_cast<double>(Width)
		, 0.0, static_cast<double>(Height)
		, 1.0, -1.0);

	//Reserve memory for our render so that we don't do 
	//excessive allocations and render the image
	OutputImage.reserve(Width * Height * 3);
	render();
}


int main(int argc, char* argv[])
{
	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
