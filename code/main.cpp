#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>

#include "shader.h"
#include "model.h"

#include <iostream>


using namespace std;
using namespace glm;

// window const
int SCR_WIDTH = 1400;
int SCR_HEIGHT = 600;
const int LEFT_CAMERA = 0;
const int RIGHT_CAMERA = 1;

//cameras
vec3 lefteye (-0.1f, 0.5, 3.0f);
vec3 righteye(0.1f, 0.5, 3.0f);
vec3 delta   (0.2, 0, 0);

vec3 left_viewat(-0.3f, 0.0f, 2);
vec3 right_viewat(-0.3f, 0.0f, 2);

int eyemode = -1;
GLfloat theta = 0.0f;       //水平旋转的角度
GLfloat viewUp = 0.0f;      //上下旋转的角度
GLfloat speed = 0.05f;
vec3 headup(0, 1, 0);

//model loaded in
vector<Model> objs;

//light parameters
vec3 light_pos = vec3(0.0f, 3.5f, 0.0f);
vec3 ambient(0.3, 0.3, 0.3);
vec3 diffuse(0.4, 0.4, 0.4);
const double lightstep = 0.05;



bool selectMode = false;
bool choosen = false;
bool changelight = false;
bool movelight = false;
bool firstRenderMouse = true;


//callback_function
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void move_mouse(GLFWwindow* window, double xpos, double ypos);
void click_mouse(GLFWwindow* window, int button, int action, int mods);
void press_key(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll(GLFWwindow* window, double x, double y);
//rendering function
void render_scene(Shader modelShader,Model background, Model sky, Model lightModel, mat4 projection, mat4 view);
void render_light(Shader lightShader, Model lightModel, mat4 projection, mat4 view);
void render_model(Shader modelShader,Model lightModel, mat4 projection,  mat4 view);

//definition reading
void parsesetting(string setting_file); 

int main()
{

	//instructions
		cout << "* instruction:\n";
		cout << "* this program aims at providing a 3D virtual world simulation \n\n";
		cout << "* move the mouse can change the view of camera and can also determine the moving direction\n";
		cout << "* w,s for camera moving into/out the sceen\n";
		cout << "* a,d for camera moving left/right\n";
		cout << "* i,k for camera moving higher/lower\n\n";
		cout << "* m for the open and close of light control\n";
		cout << "* with light control open, scroll the mouse can change the value of diffuse/ambient of the light\n";
		cout << "* mouse press on the object to drag them to somewhere you like.\n";
		cout << "* while a object is selected with the key pressed, you can rotate it with button b.\n";

		cout << "* please give us the setting file:" << endl;
	
	//read in the setting
	string file;
	cin >> file;

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif


	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "twoeye_modelling", NULL, NULL);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetCursorPosCallback(window, move_mouse);
	glfwSetMouseButtonCallback(window, click_mouse);
	glfwSetKeyCallback(window, press_key);
	glfwMakeContextCurrent(window);
	glfwSetScrollCallback(window, scroll);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	//shader loaded in
	Shader modelShader("shader/model.vs", "shader/model.fs");
	Shader lightShader("shader/light.vs", "shader/light.fs");

	Model backgroundModel("objs/background.obj");
	Model lightModel("objs/lamp.obj");
	Model sky("objs/sky.obj");


	//file = "setting.txt";
	parsesetting(file);
	

	backgroundModel.getmatrix(vec3(0.0f, -0.5f, 0.0f), vec4(0, 0, 0, 0), vec3(0.5, 0.5, 0.5));
	sky.getmatrix(vec3(0, 0, 0), vec4(0, 0, 0, 0), vec3(100, 100, 100));
	lightModel.getmatrix(light_pos, vec4(0, 0, 0, 0), vec3(1, 1, 1));
	

	//two eye rendering
	bool flush = false;
	while (!glfwWindowShouldClose(window))
	{	

		if (!flush) {
			eyemode = LEFT_CAMERA;
			glScissor(0, 0, SCR_WIDTH / 2, SCR_HEIGHT);
			glViewport(0, 0, SCR_WIDTH / 2, SCR_HEIGHT);
		}
		else {
			eyemode = RIGHT_CAMERA;
			glScissor(SCR_WIDTH / 2, 0, SCR_WIDTH / 2, SCR_HEIGHT);
			glViewport(SCR_WIDTH / 2, 0, SCR_WIDTH / 2, SCR_HEIGHT);
		}
		
		lightModel.obj_pos = light_pos;

		glClearColor(0.75f, 0.75f, 0.75f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// view/projection transformations
		mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		mat4 view;
		if(eyemode == LEFT_CAMERA)view = glm::lookAt(lefteye, left_viewat, headup);
		else view = glm::lookAt(righteye, right_viewat, headup);

		render_scene(modelShader, backgroundModel,sky, lightModel, projection, view);
		render_light(lightShader, lightModel, projection, view);
		render_model(modelShader, lightModel, projection,view);
		
		if (flush)glfwSwapBuffers(window);
		flush = !flush;
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

void parsesetting(string setting_file) {
	ifstream fin(setting_file);


	string input;
	if (!fin) {
		cout << " setting file does not exist ! " << endl;
		char c = getchar();
		exit(0);
	}
	while (!fin.eof()) {
		fin >> input;
		//element in the environment
		if (input.find(".obj") != string::npos) {
			vec3 translate, scale;
			vec4 rotate;
			Model obj(input);
			fin >> translate.x >> translate.y >> translate.z;
			fin >> rotate.x >> rotate.y >> rotate.z >> rotate.w;
			fin >> scale.x >> scale.y >> scale.z;
			//read in objs
			obj.getmatrix(translate, rotate, scale);
			objs.push_back(obj);
		}

		else if (input.find("camera") != string::npos) {
			fin >> lefteye.x >> lefteye.y >> lefteye.z;
			delta = righteye - lefteye;
		}

		else if (input.find("deltaeye") != string::npos) {
			fin >> delta.x >> delta.y >> delta.z;
			righteye = lefteye + delta;
		}

		else if (input.find("light_ambient") != string::npos) {
			fin >> ambient.x >> ambient.y >> ambient.z;
		}
		else if (input.find("light_diffuse") != string::npos) {
			fin >> diffuse.x >> diffuse.y >> diffuse.z;
		}

	}
}

void render_scene(Shader modelShader, Model background, Model sky, Model lightModel, mat4 projection, mat4 view) {
	modelShader.use();
	// be sure to activate shader when setting uniforms/drawing objects
	modelShader.setVec3("light.position", lightModel.obj_pos);

	vec3 position;
	if (eyemode == LEFT_CAMERA)position = lefteye;
	else position = righteye;

	modelShader.setVec3("viewPos", position);

	// light properties
	modelShader.setVec3("light.ambient", ambient);
	modelShader.setVec3("light.diffuse", diffuse);
	modelShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
	modelShader.setFloat("light.constant", 1.0f);
	modelShader.setFloat("light.linear", 0.09f);
	modelShader.setFloat("light.quadratic", 0.032f);

	// material properties
	modelShader.setFloat("material.shininess", 32.0f);

	modelShader.setMat4("projection", projection);
	modelShader.setMat4("view", view);

	mat4 modelTransfor = mat4(1.0f);
	modelTransfor = translate(modelTransfor, background.obj_pos); // translate it down so it's at the center of the scene
	modelTransfor = scale(modelTransfor,background.scale);
	modelShader.setMat4("model", modelTransfor);
	background.Draw(modelShader);

	mat4 skyTransfor = mat4(1.0f);
	skyTransfor = translate(skyTransfor,sky.obj_pos);
	skyTransfor = scale(skyTransfor, sky.scale);
	modelShader.setMat4("model", skyTransfor);
	sky.Draw(modelShader);

	return;
}

void render_light(Shader lightShader, Model lightModel, mat4 projection, mat4 view)
{
	mat4 lampTransfor = mat4(1.0f);

	lampTransfor = translate(lampTransfor, lightModel.obj_pos);
	lampTransfor = scale(lampTransfor, vec3(0.2f)); // a smaller cube

	lightShader.use();

	lightShader.setMat4("projection", projection);
	lightShader.setMat4("view", view);
	lightShader.setMat4("model", lampTransfor);

	lightModel.Draw(lightShader);

	return;
}

void render_model(Shader modelShader, Model lightModel, mat4 projection, mat4 view)
{
	// render the loaded model

	// don't forget to enable shader before setting uniforms
	modelShader.use();
	// be sure to activate shader when setting uniforms/drawing objects
	modelShader.setVec3("light.position", lightModel.obj_pos);

	vec3 position;
	if (eyemode == LEFT_CAMERA)position = lefteye;
	else position = righteye;

	modelShader.setVec3("viewPos", position);

	// light properties
	modelShader.setVec3("light.ambient",ambient);
	modelShader.setVec3("light.diffuse",diffuse);
	modelShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
	modelShader.setFloat("light.constant", 1.0f);
	modelShader.setFloat("light.linear", 0.1f);
	modelShader.setFloat("light.quadratic", 0.05f);

	// material properties
	modelShader.setFloat("material.shininess", 32.0f);

	modelShader.setMat4("projection", projection);
	modelShader.setMat4("view", view);

	for (int i = 0; i < objs.size(); i++) {
		mat4 modelTransfor = mat4(1.0f);

		modelTransfor = translate(modelTransfor, objs[i].obj_pos); // translate it down so it's at the center of the scene
		modelTransfor = rotate(modelTransfor, objs[i].rotate.x, vec3(objs[i].rotate.y, objs[i].rotate.z, objs[i].rotate.w));
		modelTransfor = scale(modelTransfor, objs[i].scale);
		modelShader.setMat4("model", modelTransfor);

		objs[i].Draw(modelShader);
	}

	return;
}

void press_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action != GLFW_RELEASE)exit(0);

	else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
		lefteye.x += (left_viewat.x - lefteye.x)*speed;
		lefteye.z += (left_viewat.z - lefteye.z)*speed;
	}

	else if (key == GLFW_KEY_S && action != GLFW_RELEASE) {
		lefteye.x -= (left_viewat.x - lefteye.x)*speed;
		lefteye.z -= (left_viewat.z - lefteye.z)*speed;
	}
	else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
		lefteye.x += (left_viewat.z - lefteye.z)*speed;
		lefteye.z += -(left_viewat.x - lefteye.x)*speed;
	}
	else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
		lefteye.x -= (left_viewat.z - lefteye.z)*speed;
		lefteye.z -= -(left_viewat.x - lefteye.x)*speed;
	}
	else if (key == GLFW_KEY_I && action != GLFW_RELEASE) {
		lefteye.y += speed;
	}
	else if (key == GLFW_KEY_K && action != GLFW_RELEASE) {
		lefteye.y -= speed;
	}
	else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		changelight = !changelight;
	}
	else if (key == GLFW_KEY_B && action != GLFW_RELEASE) {
		if (selectMode && !movelight) {
			for (int i = 0; i < objs.size(); i++) {
				if (objs[i].obj_choosen == true) {
					objs[i].rotate.x += 0.05;
				}
			}
		}
	}
	else if (key == GLFW_KEY_N && action == GLFW_PRESS) {
		if (selectMode == false) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		selectMode = !selectMode;
		firstRenderMouse = !firstRenderMouse;
	}
	righteye = lefteye + delta;


	left_viewat.x = float(lefteye.x + cos(theta));					 // 新的参考点的位置    
	left_viewat.z = float(lefteye.z + sin(theta));
	left_viewat.y = float(lefteye.y + viewUp);

	right_viewat = left_viewat;
}

void click_mouse(GLFWwindow* window, int button, int action, int mods)
{
	if (!selectMode) {
		return;
	}

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	if (mouseX < SCR_WIDTH / 2.0f) {
		eyemode = LEFT_CAMERA;
	}
	else if (mouseX > SCR_WIDTH / 2.0f) {
		eyemode = RIGHT_CAMERA;
	}

	mat4 projection = perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	mat4 view;
	if (eyemode == LEFT_CAMERA)view = glm::lookAt(lefteye, left_viewat, headup);
	else view = glm::lookAt(righteye, right_viewat, headup);

	float x, y;
	if (mouseX < SCR_WIDTH / 2.0f) {
		x = 4.0f * mouseX / SCR_WIDTH - 1.0f;
		y = 1.0f - 2.0f * mouseY / SCR_HEIGHT;
	}
	else {
		x = 4.0f * mouseX / SCR_WIDTH - 3.0f;
		y = 1.0f - 2.0f * mouseY / SCR_HEIGHT;
	}

	double minDis = 0.1;
	static int minIndex = -1;
	for (int i = 0; i < objs.size() ; i++) {
		vec4 pos = projection * view * translate(mat4(1.0f), objs[i].obj_pos) * vec4(0.0, 0.0, 0.0, 1.0);
		double dis = (pos.x / pos.w - x) * (pos.x / pos.w - x)
			+ (pos.y / pos.w - y) * (pos.y / pos.w - y);

		dis = dis / objs[i].scale[0];

		if (abs(dis) < minDis) {
			minDis = abs(dis);
			minIndex = i;
		}
	}

	vec4 pos = projection * view * translate(mat4(1.0f), light_pos) * vec4(0.0, 0.0, 0.0, 1.0);
	double dis = (pos.x / pos.w - x) * (pos.x / pos.w - x)
		+ (pos.y / pos.w - y) * (pos.y / pos.w - y);
	if (abs(dis) < minDis) {
		minDis = abs(dis);
		movelight = true;
	}
	if (minIndex != -1 && !movelight ) {
		objs[minIndex].obj_choosen = true;
	}
	if (action == GLFW_RELEASE && (minIndex != -1 || movelight == true)) {
		if (minIndex != -1) {
			objs[minIndex].obj_choosen = false;
			cout << objs[minIndex].obj_pos.x << objs[minIndex].obj_pos.y << objs[minIndex].obj_pos.z << endl;
		}

		movelight = false;
		minIndex = -1;
	}

	return;
}

void move_mouse(GLFWwindow* window, double xpos, double ypos)
{

	static float lastXpos, lastYpos;//for drag

	static float lastX = SCR_WIDTH / 2;//for move
	static float lastY = SCR_HEIGHT / 2;

	if (selectMode) {
		if (!selectMode) {
			lastXpos = xpos;
			lastYpos = ypos;
		}

		float xoffset = xpos - lastXpos;
		float yoffset = lastYpos - ypos; 
		
		xoffset /= SCR_WIDTH / 8;
		yoffset /= SCR_HEIGHT / 8;
		
		lastXpos = xpos;
		lastYpos = ypos;

		if (movelight) {
			vec3 offset(-(left_viewat.z - lefteye.z)* xoffset, yoffset/5, (left_viewat.x - lefteye.x)* xoffset);
			light_pos += offset;
			return;
		}

		for (int i = 0; i < objs.size(); i++) {
			if (objs[i].obj_choosen == true) {
				vec3 offset(-(left_viewat.z - lefteye.z)* xoffset, yoffset/5, (left_viewat.x - lefteye.x)* xoffset);
				objs[i].obj_pos += offset;
				break;
			}
		}
	}
	
	else {
		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		theta += GLfloat(xoffset) / 1000;      //旋转改变量
	

		viewUp += GLfloat(yoffset) / 1000;     //上下改变量
		if (viewUp <= -80)viewUp = -80;
		if (viewUp >= 80)viewUp = 80;

		left_viewat.x = float(lefteye.x + cos(theta));					 // 新的参考点的位置    
		left_viewat.z = float(lefteye.z + sin(theta));
		left_viewat.y = float(lefteye.y + viewUp);

		right_viewat = left_viewat;
	}
	return;
}

void scroll(GLFWwindow* window, double x, double y) {
	if (changelight) {

		ambient += vec3(lightstep * y, lightstep * y, lightstep * y);
		diffuse += vec3(lightstep * y, lightstep * y, lightstep * y);

		if (std::min(std::min(ambient.x, ambient.y), ambient.z) <= 0.05) ambient = vec3(0.05, 0.05, 0.05);
		if (std::max(std::max(ambient.x, ambient.y), ambient.z) >= 0.95) ambient = vec3(0.95, 0.95, 0.95);

		if (std::min(std::min(diffuse.x, diffuse.y), diffuse.z) <= 0.05) diffuse = vec3(0.05, 0.05, 0.05);
		if (std::max(std::max(diffuse.x, diffuse.y), diffuse.z) >= 0.95) diffuse = vec3(0.95, 0.95, 0.95);

	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
}