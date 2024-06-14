#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <stb_image.h>
#include "Model.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480


struct GameObject
{
	glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotation = glm::vec3(0.f);
	glm::vec3 scale = glm::vec3(1.0f);
	glm::vec3 forward = glm::vec3(0.f, 1.f, 0.f);
	float fVelocity = 0.01f;
	float fAngularVelocity = 1.f;
	float fScaleVelocity = 0.01f;

};

struct Camera 
{
	glm::vec3 position = glm::vec3(0.f,2.f,5.f);
	glm::vec3 direction = position + glm::vec3(0.f,0.f,-10.f);
	glm::vec3 localVectorUp = glm::vec3(0.f,1.f,0.f);
	glm::vec3 localVectorFront = glm::vec3(0.f,0.f,-1.f);
	glm::vec3 initialPosition = glm::vec3(0.f, 2.f, 5.f);
	glm::vec3 offset = glm::vec3(0.f, 0.f, 0.5f);
	float dollyZoomSpeed = 0.01f;
	float fovSpeed = 0.1f;
	float initialFov = 45.f;
	float fFov = 45.f;
	float fNear = 0.1f;
	float fFar = 1000.f;
	float aspectRatio;
	float pitch = 0;
	float yaw = 0;
	glm::vec3 cameraFront = glm::vec3(0.f, 0.f, -1.f);
};
Camera camara;

struct Star
{
	glm::vec3 position;
	float fAngularVelocity = 0.0000000005f;

	float angle = 0.5f;
};
Star sun;
Star moon;

struct SpawnPoint
{
	glm::vec3 position;
	glm::vec3 rotation;
	float angle;

};


struct ShaderProgram {
	GLuint vertexShader = 0;
	GLuint geometryShader = 0;
	GLuint fragmentShader = 0;
};

std::vector<GLuint> compiledPrograms;
std::vector<Model> models;
std::vector<SpawnPoint> spawnPoints;
std::vector<SpawnPoint> modelPoints;

bool isometric = true;
bool generalPlane = false;
bool detailPlane = false;
bool dollyZoom = false;
bool first = true;
bool linterna = false;
bool keyFPressed = false;
bool sunLight = true;
bool moonLight = false;
bool noche = false;
float lastX = 400.0f;
float lastY = 300.0f;
float camaraSpeed = 0.05f;
bool firstMouse = true;

void Resize_Window(GLFWwindow* window, int iFrameBufferWidth, int iFrameBufferHeight) {

	//Definir nuevo tamaño del viewport
	glViewport(0, 0, iFrameBufferWidth, iFrameBufferHeight);
	glUniform2f(glGetUniformLocation(compiledPrograms[0], "windowSize"), iFrameBufferWidth, iFrameBufferHeight);
}

glm::mat4 GenerateTranslationMatrix(glm::vec3 translation)
{
	return glm::translate(glm::mat4(1.0f), translation);

}

glm::mat4 GenerateRotationMatrix(glm::vec3 axis, float fDegrees)
{
	return glm::rotate(glm::mat4(1.0f), glm::radians(fDegrees), glm::normalize(axis));

}

glm::mat4 GenerateScaleMatrix(glm::vec3 scaleAxis)
{
	return glm::scale(glm::mat4(1.0f), scaleAxis);

}

void keyEvents(GLFWwindow* window, int key, int scancode, int action, int mods)
{


}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	camara.yaw += xoffset;
	camara.pitch += yoffset;

	if (camara.pitch > 89.0f)
		camara.pitch = 89.0f;
	if (camara.pitch < -89.0f)
		camara.pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(camara.yaw)) * cos(glm::radians(camara.pitch));
	direction.y = sin(glm::radians(camara.pitch));
	direction.z = sin(glm::radians(camara.yaw)) * cos(glm::radians(camara.pitch));
	camara.direction = camara.position + glm::normalize(direction);
	camara.localVectorFront = glm::normalize(direction);

}

void UpdateSunRotation(float deltaTime) {
	// Define el punto alrededor del cual rotará el Sol (por ejemplo, el centro del sistema de coordenadas)
	glm::vec3 rotationCenter = glm::vec3(0.f, 1.f, -1.f);

	// Define el eje y el ángulo de rotación
	glm::vec3 rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f); // Rotación alrededor del eje Y
	float rotationAngle = sun.angle + sun.fAngularVelocity * deltaTime; // Calcula el nuevo ángulo

	// Paso 1: Traslada el Sol al origen del sistema de coordenadas
	glm::mat4 translateToOrigin = GenerateTranslationMatrix(-rotationCenter);

	// Paso 2: Aplica la rotación
	glm::mat4 rotation = GenerateRotationMatrix(rotationAxis, rotationAngle);

	// Paso 3: Devuelve el Sol a su posición original
	glm::mat4 translateBack = GenerateTranslationMatrix(rotationCenter);

	// Combina las transformaciones
	glm::mat4 transform = translateBack * rotation * translateToOrigin;

	// Aplica la transformación a la posición del Sol
	sun.position = glm::vec3(transform * glm::vec4(sun.position, 1.0f));


	// Actualiza el ángulo del Sol
	sun.angle = rotationAngle;
}

void UpdateMoonRotation(float deltaTime) {
	// Define el punto alrededor del cual rotará el Sol (por ejemplo, el centro del sistema de coordenadas)
	glm::vec3 rotationCenter = glm::vec3(0.f, 1.f, -1.f);

	// Define el eje y el ángulo de rotación
	glm::vec3 rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f); // Rotación alrededor del eje Y
	float rotationAngle = moon.angle + moon.fAngularVelocity * deltaTime; // Calcula el nuevo ángulo

	// Paso 1: Traslada el Sol al origen del sistema de coordenadas
	glm::mat4 translateToOrigin = GenerateTranslationMatrix(-rotationCenter);

	// Paso 2: Aplica la rotación
	glm::mat4 rotation = GenerateRotationMatrix(rotationAxis, rotationAngle);

	// Paso 3: Devuelve el Sol a su posición original
	glm::mat4 translateBack = GenerateTranslationMatrix(rotationCenter);

	// Combina las transformaciones
	glm::mat4 transform = translateBack * rotation * translateToOrigin;

	// Aplica la transformación a la posición del Sol
	moon.position = glm::vec3(transform * glm::vec4(moon.position, 1.0f));


	// Actualiza el ángulo del Sol
	moon.angle = rotationAngle;
}

SpawnPoint RandomSpawnpoint(std::vector<SpawnPoint> spawnPoints)
{
	

	// Generate a random index within the bounds of the vector
	int randomIndex = std::rand() % spawnPoints.size();

	// Return the spawn point at the random index
	return spawnPoints[randomIndex];


}


//Funcion que leera un .obj y devolvera un modelo para poder ser renderizado
Model LoadOBJModel(const std::string& filePath) {

	//Verifico archivo y si no puedo abrirlo cierro aplicativo
	std::ifstream file(filePath);

	if (!file.is_open()) {
		std::cerr << "No se ha podido abrir el archivo: " << filePath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	//Variables lectura fichero
	std::string line;
	std::stringstream ss;
	std::string prefix;
	glm::vec3 tmpVec3;
	glm::vec2 tmpVec2;

	//Variables elemento modelo
	std::vector<float> vertexs;
	std::vector<float> vertexNormal;
	std::vector<float> textureCoordinates;

	//Variables temporales para algoritmos de sort
	std::vector<float> tmpVertexs;
	std::vector<float> tmpNormals;
	std::vector<float> tmpTextureCoordinates;

	//Recorremos archivo linea por linea
	while (std::getline(file, line)) {

		//Por cada linea reviso el prefijo del archivo que me indica que estoy analizando
		ss.clear();
		ss.str(line);
		ss >> prefix;

		//Estoy leyendo un vertice
		if (prefix == "v") {

			//Asumo que solo trabajo 3D así que almaceno XYZ de forma consecutiva
			ss >> tmpVec3.x >> tmpVec3.y >> tmpVec3.z;

			//Almaceno en mi vector de vertices los valores
			tmpVertexs.push_back(tmpVec3.x);
			tmpVertexs.push_back(tmpVec3.y);
			tmpVertexs.push_back(tmpVec3.z);
		}

		//Estoy leyendo una UV (texture coordinate)
		else if (prefix == "vt") {

			//Las UVs son siempre imagenes 2D asi que uso el tmpvec2 para almacenarlas
			ss >> tmpVec2.x >> tmpVec2.y;

			//Almaceno en mi vector temporal las UVs
			tmpTextureCoordinates.push_back(tmpVec2.x);
			tmpTextureCoordinates.push_back(tmpVec2.y);

		}

		//Estoy leyendo una normal
		else if (prefix == "vn") {

			//Asumo que solo trabajo 3D así que almaceno XYZ de forma consecutiva
			ss >> tmpVec3.x >> tmpVec3.y >> tmpVec3.z;

			//Almaceno en mi vector temporal de normales las normales
			tmpNormals.push_back(tmpVec3.x);
			tmpNormals.push_back(tmpVec3.y);
			tmpNormals.push_back(tmpVec3.z);

		}

		//Estoy leyendo una cara
		else if (prefix == "f") {

			int vertexData;
			short counter = 0;

			//Obtengo todos los valores hasta un espacio
			while (ss >> vertexData) {

				//En orden cada numero sigue el patron de vertice/uv/normal
				switch (counter) {
				case 0:
					//Si es un vertice lo almaceno - 1 por el offset y almaceno dos seguidos al ser un vec3, salto 1 / y aumento el contador en 1
					vertexs.push_back(tmpVertexs[(vertexData - 1) * 3]);
					vertexs.push_back(tmpVertexs[((vertexData - 1) * 3) + 1]);
					vertexs.push_back(tmpVertexs[((vertexData - 1) * 3) + 2]);
					ss.ignore(1, '/');
					counter++;
					break;
				case 1:
					//Si es un uv lo almaceno - 1 por el offset y almaceno dos seguidos al ser un vec2, salto 1 / y aumento el contador en 1
					textureCoordinates.push_back(tmpTextureCoordinates[(vertexData - 1) * 2]);
					textureCoordinates.push_back(tmpTextureCoordinates[((vertexData - 1) * 2) + 1]);
					ss.ignore(1, '/');
					counter++;
					break;
				case 2:
					//Si es una normal la almaceno - 1 por el offset y almaceno tres seguidos al ser un vec3, salto 1 / y reinicio
					vertexNormal.push_back(tmpNormals[(vertexData - 1) * 3]);
					vertexNormal.push_back(tmpNormals[((vertexData - 1) * 3) + 1]);
					vertexNormal.push_back(tmpNormals[((vertexData - 1) * 3) + 2]);
					counter = 0;
					break;
				}
			}
		}
	}
	return Model(vertexs, textureCoordinates, vertexNormal);
}


//Funcion que devolvera una string con todo el archivo leido
std::string Load_File(const std::string& filePath) {

	std::ifstream file(filePath);

	std::string fileContent;
	std::string line;

	//Lanzamos error si el archivo no se ha podido abrir
	if (!file.is_open()) {
		std::cerr << "No se ha podido abrir el archivo: " << filePath << std::endl;
		std::exit(EXIT_FAILURE);
	}

	//Leemos el contenido y lo volcamos a la variable auxiliar
	while (std::getline(file, line)) {
		fileContent += line + "\n";
	}

	//Cerramos stream de datos y devolvemos contenido
	file.close();

	return fileContent;
}

GLuint LoadFragmentShader(const std::string& filePath) {

	// Crear un fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	//Usamos la funcion creada para leer el fragment shader y almacenarlo 
	std::string sShaderCode = Load_File(filePath);
	const char* cShaderSource = sShaderCode.c_str();

	//Vinculamos el fragment shader con su código fuente
	glShaderSource(fragmentShader, 1, &cShaderSource, nullptr);

	// Compilar el fragment shader
	glCompileShader(fragmentShader);

	// Verificar errores de compilación
	GLint success;
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	//Si la compilacion ha sido exitosa devolvemos el fragment shader
	if (success) {

		return fragmentShader;

	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);

		//Obtenemos el log
		std::vector<GLchar> errorLog(logLength);
		glGetShaderInfoLog(fragmentShader, logLength, nullptr, errorLog.data());

		//Mostramos el log y finalizamos programa
		std::cerr << "Se ha producido un error al cargar el fragment shader:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}


GLuint LoadGeometryShader(const std::string& filePath) {

	// Crear un vertex shader
	GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);

	//Usamos la funcion creada para leer el vertex shader y almacenarlo 
	std::string sShaderCode = Load_File(filePath);
	const char* cShaderSource = sShaderCode.c_str();

	//Vinculamos el vertex shader con su código fuente
	glShaderSource(geometryShader, 1, &cShaderSource, nullptr);

	// Compilar el vertex shader
	glCompileShader(geometryShader);

	// Verificar errores de compilación
	GLint success;
	glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);

	//Si la compilacion ha sido exitosa devolvemos el vertex shader
	if (success) {

		return geometryShader;

	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &logLength);

		//Obtenemos el log
		std::vector<GLchar> errorLog(logLength);
		glGetShaderInfoLog(geometryShader, logLength, nullptr, errorLog.data());

		//Mostramos el log y finalizamos programa
		std::cerr << "Se ha producido un error al cargar el vertex shader:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

GLuint LoadVertexShader(const std::string& filePath) {

	// Crear un vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

	//Usamos la funcion creada para leer el vertex shader y almacenarlo 
	std::string sShaderCode = Load_File(filePath);
	const char* cShaderSource = sShaderCode.c_str();

	//Vinculamos el vertex shader con su código fuente
	glShaderSource(vertexShader, 1, &cShaderSource, nullptr);

	// Compilar el vertex shader
	glCompileShader(vertexShader);

	// Verificar errores de compilación
	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	//Si la compilacion ha sido exitosa devolvemos el vertex shader
	if (success) {

		return vertexShader;

	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);

		//Obtenemos el log
		std::vector<GLchar> errorLog(logLength);
		glGetShaderInfoLog(vertexShader, logLength, nullptr, errorLog.data());

		//Mostramos el log y finalizamos programa
		std::cerr << "Se ha producido un error al cargar el vertex shader:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

//Función que dado un struct que contiene los shaders de un programa generara el programa entero de la GPU
GLuint CreateProgram(const ShaderProgram& shaders) {

	//Crear programa de la GPU
	GLuint program = glCreateProgram();

	//Verificar que existe un vertex shader y adjuntarlo al programa
	if (shaders.vertexShader != 0) {
		glAttachShader(program, shaders.vertexShader);
	}

	if (shaders.geometryShader != 0) {
		glAttachShader(program, shaders.geometryShader);
	}

	if (shaders.fragmentShader != 0) {
		glAttachShader(program, shaders.fragmentShader);
	}

	// Linkear el programa
	glLinkProgram(program);

	//Obtener estado del programa
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	//Devolver programa si todo es correcto o mostrar log en caso de error
	if (success) {

		//Liberamos recursos
		if (shaders.vertexShader != 0) {
			glDetachShader(program, shaders.vertexShader);
		}

		//Liberamos recursos
		if (shaders.geometryShader != 0) {
			glDetachShader(program, shaders.geometryShader);
		}

		//Liberamos recursos
		if (shaders.fragmentShader != 0) {
			glDetachShader(program, shaders.fragmentShader);
		}

		return program;
	}
	else {

		//Obtenemos longitud del log
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

		//Almacenamos log
		std::vector<GLchar> errorLog(logLength);
		glGetProgramInfoLog(program, logLength, nullptr, errorLog.data());

		std::cerr << "Error al linkar el programa:  " << errorLog.data() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void main() {

	//Definir semillas del rand según el tiempo
	srand(static_cast<unsigned int>(time(NULL)));

	//Inicializamos GLFW para gestionar ventanas e inputs
	glfwInit();

	//Configuramos la ventana
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	//Inicializamos la ventana
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "My Engine", NULL, NULL);

	//Asignamos función de callback para cuando el frame buffer es modificado
	glfwSetFramebufferSizeCallback(window, Resize_Window);

	//Definimos espacio de trabajo
	glfwMakeContextCurrent(window);

	//Permitimos a GLEW usar funcionalidades experimentales
	glewExperimental = GL_TRUE;

	//Activamos cull face
	glEnable(GL_CULL_FACE);

	//Indicamos lado del culling
	glCullFace(GL_BACK);

	//Indicamos lado del culling
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_POLYGON_OFFSET_FILL);


	//Leer textura
	int width, height, nrChannels;

	//Inicializamos GLEW y controlamos errores
	if (glewInit() == GLEW_OK) {

		//genero spawnpoints
		SpawnPoint point0,point1,point2,point3,point4;
		//punto 0
		point0.position = glm::vec3(1.5f, 1.f, 0.f);
		point0.rotation = glm::vec3(0.f, 1.f, 0.f);
		point0.angle = -75.0f;
		//punto 1
		point1.position = glm::vec3(-1.5f, 1.f, 0.f);
		point1.rotation = glm::vec3(0.f, 1.f, 0.f);
		point1.angle = 75.0f;
		//punto 2
		point2.position = glm::vec3(1.f, 1.f, -2.f);
		point2.rotation = glm::vec3(0.f, 1.f, 0.f);
		point2.angle = 45.0f;
		//punto 3
		point3.position = glm::vec3(0.0f, 1.f, 1.f);
		point3.rotation = glm::vec3(0.f, 1.f, 0.f);
		point3.angle = 30.0f;
		//punto 4
		point4.position = glm::vec3(-1.f, 1.f, 2.f);
		point4.rotation = glm::vec3(0.f, 1.f, 0.f);
		point2.angle = -45.0f;

		spawnPoints.push_back(point0);
		spawnPoints.push_back(point1);
		spawnPoints.push_back(point2);
		spawnPoints.push_back(point3);
		spawnPoints.push_back(point4);


		//Compilar shaders
		ShaderProgram illuminationProgram;
		illuminationProgram.vertexShader = LoadVertexShader("MyFirstVertexShader.glsl");
		illuminationProgram.geometryShader = LoadGeometryShader("MyFirstGeometryShader.glsl");
		illuminationProgram.fragmentShader = LoadFragmentShader("MyFirstFragmentShader.glsl");


		unsigned char* textureInfo = stbi_load("Assets/Textures/troll.png", &width, &height, &nrChannels, 0);
		//Cargo Modelo
		models.push_back(LoadOBJModel("Assets/Models/troll.obj"));

		//Compìlar programa
		compiledPrograms.push_back(CreateProgram(illuminationProgram));

		//Definimos canal de textura activo
		glActiveTexture(GL_TEXTURE0);

		//Generar textura
		GLuint textureID;
		glGenTextures(1, &textureID);

		//Vinculamos texture
		glBindTexture(GL_TEXTURE_2D, textureID);

		//Configurar textura
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Cargar imagen a la textura
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureInfo);

		//Generar mipmap
		glGenerateMipmap(GL_TEXTURE_2D);

		//Liberar memoria de la imagen cargada
		stbi_image_free(textureInfo);

		//Definimos color para limpiar el buffer de color
		glClearColor(0.f, 255.f, 255.f, 1.f);

		//Definimos modo de dibujo para cada cara
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//Indicar a la tarjeta GPU que programa debe usar
		glUseProgram(compiledPrograms[0]);

		//MODELO 2
		unsigned char* textureInfoRock = stbi_load("Assets/Textures/rock.png", &width, &height, &nrChannels, 0);
		//Cargo Modelo
		models.push_back(LoadOBJModel("Assets/Models/rock.obj"));

		//Definimos canal de textura activo
		glActiveTexture(GL_TEXTURE1);

		//Generar textura
		GLuint textureID2;
		glGenTextures(1, &textureID2);

		//Vinculamos texture
		glBindTexture(GL_TEXTURE_2D, textureID2);

		//Configurar textura
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Cargar imagen a la textura
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureInfoRock);

		//Generar mipmap
		glGenerateMipmap(GL_TEXTURE_2D);

		//Liberar memoria de la imagen cargada
		stbi_image_free(textureInfoRock);

		//Definimos color para limpiar el buffer de color
		glClearColor(0.f, 255.f, 255.f, 1.f);

		//Definimos modo de dibujo para cada cara
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//Indicar a la tarjeta GPU que programa debe usar
		glUseProgram(compiledPrograms[0]);

		//MODELO BOTAMON
		unsigned char* textureInfoBota = stbi_load("Assets/Textures/grass.png", &width, &height, &nrChannels, 0);
		//Cargo Modelo
		models.push_back(LoadOBJModel("Assets/Models/grass.obj"));

		//Definimos canal de textura activo
		glActiveTexture(GL_TEXTURE2);

		//Generar textura
		GLuint textureID3;
		glGenTextures(1, &textureID3);

		//Vinculamos texture
		glBindTexture(GL_TEXTURE_2D, textureID3);

		//Configurar textura
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Cargar imagen a la textura
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureInfoBota);

		//Generar mipmap
		glGenerateMipmap(GL_TEXTURE_2D);

		//Liberar memoria de la imagen cargada
		stbi_image_free(textureInfoBota);

		//Definimos color para limpiar el buffer de color
		glClearColor(0.f, 255.f, 255.f, 1.f);

		//Definimos modo de dibujo para cada cara
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//Indicar a la tarjeta GPU que programa debe usar
		glUseProgram(compiledPrograms[0]);


		ShaderProgram ColorProgram;
		ColorProgram.vertexShader = LoadVertexShader("MyFirstVertexShader.glsl");
		ColorProgram.geometryShader = LoadGeometryShader("MyFirstGeometryShader.glsl");
		ColorProgram.fragmentShader = LoadFragmentShader("FragmentShaderColor.glsl");

		//Compìlar programa
		compiledPrograms.push_back(CreateProgram(ColorProgram));



		float lastFrameTime = glfwGetTime();
		sun.position = glm::vec3(0.0f, 0.0f, 10.0f);
		moon.position = glm::vec3(0.0f, 0.0f, -10.0f);

		SpawnPoint modelPoint = RandomSpawnpoint(spawnPoints);
		SpawnPoint modelPoint1 = RandomSpawnpoint(spawnPoints);
		SpawnPoint modelPoint2 = RandomSpawnpoint(spawnPoints);
		SpawnPoint modelPoint3 = RandomSpawnpoint(spawnPoints);

		modelPoints.push_back(modelPoint);
		modelPoints.push_back(modelPoint1);
		modelPoints.push_back(modelPoint2);
		modelPoints.push_back(modelPoint3);


		//Generamos el game loop
		while (!glfwWindowShouldClose(window)) {

			
			//Pulleamos los eventos (botones, teclas, mouse...)
			glfwPollEvents();
			glfwSetKeyCallback(window, keyEvents);
			glfwMakeContextCurrent(window);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorPosCallback(window, mouse_callback);



			float currentFrameTime = glfwGetTime();
			float deltaTime = currentFrameTime - lastFrameTime;
			UpdateSunRotation(deltaTime);

			if (sun.position.y < -5.0f) 
			{
				sunLight = false;
				noche = true;
			}
			else if (sun.position.y > -5.0f) 
			{
				sunLight = true;
			}

				UpdateMoonRotation(deltaTime);

			if (moon.position.y < -5.0f)
			{
				moonLight = false;
				
			}
			else if (moon.position.y > -5.0f)
			{
				moonLight = true;
			}


			int state = glfwGetKey(window, GLFW_KEY_F);

			// Comprueba si la tecla F está presionada en este ciclo y no lo estaba en el ciclo anterior
			if (state == GLFW_PRESS && !keyFPressed)
			{
				// Cambia el estado de la linterna
				linterna = !linterna;

				// Actualiza el estado de la tecla F a presionada
				keyFPressed = true;
			}

			// Comprueba si la tecla F no está presionada para actualizar el estado de la tecla
			if (state == GLFW_RELEASE)
			{
				// Actualiza el estado de la tecla F a no presionada
				keyFPressed = false;
			}

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
			{
				camara.position += camaraSpeed * camara.localVectorFront;
				/*camara.position.z -= 0.05f;
				camara.direction.z -= 0.05f;*/
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			{
				camara.position -= camaraSpeed * camara.localVectorFront;
				/*camara.position.z += 0.05f;
				camara.direction.z += 0.05f;*/
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{
				camara.position += glm::normalize(glm::cross(camara.localVectorFront, camara.localVectorUp)) * camaraSpeed;
				/*camara.position.x += 0.05f;
				camara.direction.x += 0.05f*/;
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{
				camara.position -= glm::normalize(glm::cross(camara.localVectorFront, camara.localVectorUp)) * camaraSpeed;
				/*camara.position.x -= 0.05f;
				camara.direction.x -= 0.05f;*/
			}

			//Limpiamos los buffers
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			glUniform2f(glGetUniformLocation(compiledPrograms[1], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);
			glUseProgram(compiledPrograms[0]);
			// <<<<<<<<<<<<<<<<<<<<<<<<<<TROLL ORIGINAL>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "textureSampler"), 0);

			//Definir la matriz de traslacion, rotacion y escalado
			glm::mat4 translationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0.f,1.f,-1.f));
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.f, 1.f, 0.f));
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.5f));

			// Definir la matriz de vista
			glm::mat4 view = glm::lookAt(camara.position, camara.position + camara.localVectorFront, camara.localVectorUp);
			//glm::mat4 view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);
			view = view * GenerateTranslationMatrix(camara.offset);

			// Definir la matriz proyeccion
			glm::mat4 projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);

			//Asignar valores iniciales al programa
			glUniform2f(glGetUniformLocation(compiledPrograms[0], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);


			// Pasar las matrices
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(translationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "linterna"), linterna);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "sun"), sunLight);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "moon"), moonLight);
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "sunLight"), 1, glm::value_ptr(sun.position));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLight"), 1, glm::value_ptr(camara.position));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLightDirection"), 1, glm::value_ptr(camara.direction));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "moonLight"), 1, glm::value_ptr(moon.position));
			glPolygonOffset(1.0f, 1.0f);
			models[0].Render();
			glDisable(GL_POLYGON_OFFSET_FILL);

			// <<<<<<<<<<<<<<<<<<<<<<<<<<GRASS>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

			
			glUseProgram(compiledPrograms[0]);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, textureID3);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "textureSampler"), 2);

			//Definir la matriz de traslacion, rotacion y escalado
			 translationMatrix = glm::translate(glm::mat4(1.f), modelPoints[0].position);
			 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(modelPoints[0].angle), modelPoints[0].rotation);
			 scaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.05f));

			// Definir la matriz de vista
			 //view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

			 // Definir la matriz proyeccion
			 projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);

			 

			 //Asignar valores iniciales al programa
			 glUniform2f(glGetUniformLocation(compiledPrograms[0], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);

			 // Pasar las matrices
			 glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(translationMatrix));
			 glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
			 glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix));
			 glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "view"), 1, GL_FALSE, glm::value_ptr(view));
			 glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			 glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			 glUniform1i(glGetUniformLocation(compiledPrograms[0], "linterna"), linterna);
			 glUniform1i(glGetUniformLocation(compiledPrograms[0], "sun"), sunLight);
			 glUniform1i(glGetUniformLocation(compiledPrograms[0], "moon"), moonLight);
			 glUniform3fv(glGetUniformLocation(compiledPrograms[0], "sunLight"), 1, glm::value_ptr(sun.position));
			 glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLight"), 1, glm::value_ptr(camara.position));
			 glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLightDirection"), 1, glm::value_ptr(camara.direction));
			 glUniform3fv(glGetUniformLocation(compiledPrograms[0], "moonLight"), 1, glm::value_ptr(moon.position));
			 glPolygonOffset(1.0f, 1.0f);
			 models[2].Render();


			// <<<<<<<<<<<<<<<<<<<<<<<<<<GRASS 2>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			 glUseProgram(compiledPrograms[0]);
			 glActiveTexture(GL_TEXTURE2);
			 glBindTexture(GL_TEXTURE_2D, textureID3);
			 glUniform1i(glGetUniformLocation(compiledPrograms[0], "textureSampler"), 2);

			 //Definir la matriz de traslacion, rotacion y escalado
			 translationMatrix = glm::translate(glm::mat4(1.f), modelPoints[1].position);
			 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(modelPoints[1].angle), modelPoints[1].rotation);
			 scaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.05f));

			// Definir la matriz de vista
			//view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

			// Definir la matriz proyeccion
			projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);

			//Asignar valores iniciales al programa
			glUniform2f(glGetUniformLocation(compiledPrograms[0], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);


			// Pasar las matrices
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(translationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "linterna"), linterna);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "sun"), sunLight);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "moon"), moonLight);
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "sunLight"), 1, glm::value_ptr(sun.position));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLight"), 1, glm::value_ptr(camara.position));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLightDirection"), 1, glm::value_ptr(camara.direction));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "moonLight"), 1, glm::value_ptr(moon.position));
			glPolygonOffset(1.0f, 1.0f);
			models[2].Render();



			//// <<<<<<<<<<<<<<<<<<<<<<<<<<PIEDRA ORIGINAL>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			glUseProgram(compiledPrograms[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, textureID2);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "textureSampler"), 1);

			//Definir la matriz de traslacion, rotacion y escalado
			glm::mat4 translationMatrix2 = glm::translate(glm::mat4(1.f), glm::vec3(-1.8f, 0.80f, -0.8f));
			glm::mat4 rotationMatrix2 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.f, 1.f, 0.f));
			glm::mat4 scaleMatrix2 = glm::scale(glm::mat4(1.f), glm::vec3(6.f, 0.2f, 6.f));

			// Definir la matriz de vista
			//view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

			// Definir la matriz proyeccion
			projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);

			//Asignar valores iniciales al programa
			glUniform2f(glGetUniformLocation(compiledPrograms[0], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);

			// Pasar las matrices
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(translationMatrix2));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix2));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix2));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[0], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "linterna"), linterna);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "sun"), sunLight);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "moon"), moonLight);
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "sunLight"), 1, glm::value_ptr(sun.position));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLight"), 1, glm::value_ptr(camara.position));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "flashLightDirection"), 1, glm::value_ptr(camara.direction));
			glUniform3fv(glGetUniformLocation(compiledPrograms[0], "moonLight"), 1, glm::value_ptr(moon.position));
			glPolygonOffset(1.0f, 1.0f);
			models[1].Render();


			//Cambiamos buffers
			glFlush();
			glfwSwapBuffers(window);
		}

		//Desactivar y eliminar programa
		glUseProgram(0);
		glDeleteProgram(compiledPrograms[0]);

	}
	else {
		std::cout << "Ha petao." << std::endl;
		glfwTerminate();
	}

	//Finalizamos GLFW
	glfwTerminate();

}