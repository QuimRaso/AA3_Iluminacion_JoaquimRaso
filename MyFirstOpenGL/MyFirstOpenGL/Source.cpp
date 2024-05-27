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
	glm::vec3 direction = position + glm::vec3(0.f,0.f,-1.f);
	glm::vec3 localVectorUp = glm::vec3(0.f,1.f,0.f);
	glm::vec3 initialPosition = glm::vec3(0.f, 2.f, 5.f);
	float dollyZoomSpeed = 0.01f;
	float fovSpeed = 0.1f;
	float initialFov = 45.f;
	float fFov = 45.f;
	float fNear = 0.1f;
	float fFar = 10.f;
	float aspectRatio;
};

struct ShaderProgram {
	GLuint vertexShader = 0;
	GLuint geometryShader = 0;
	GLuint fragmentShader = 0;
};

std::vector<GLuint> compiledPrograms;
std::vector<Model> models;
GameObject cube;
bool isometric = true;
bool generalPlane = false;
bool detailPlane = false;
bool dollyZoom = false;
bool first = true;

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


	if ((key == GLFW_KEY_1 && action == GLFW_PRESS))
	{

		if (generalPlane == true) 
		{
			generalPlane = false;
			first = true;
			isometric = true;
			
		}
		else 
		{
			isometric = false;
			detailPlane = false;
			generalPlane = true;
		}

	}

	if ((key == GLFW_KEY_2 && action == GLFW_PRESS))
	{
		if (detailPlane == true)
		{
			detailPlane = false;
			first = true;
			isometric = true;
		}
		else
		{
			detailPlane = true;
			isometric = false;
			generalPlane = false;
		}


	}

	if ((key == GLFW_KEY_3 && action == GLFW_PRESS))
	{
		
		isometric = false;
		detailPlane = false;
		generalPlane = false;
		first = true;
		dollyZoom = true;

	}


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

	//Leer textura
	int width, height, nrChannels;
	unsigned char* textureInfo = stbi_load("Assets/Textures/troll.png", &width, &height, &nrChannels, 0);
	unsigned char* textureInfoRock = stbi_load("Assets/Textures/rock.png", &width, &height, &nrChannels, 0);

	//Inicializamos GLEW y controlamos errores
	if (glewInit() == GLEW_OK) {

		
		Camera camara;

		//Compilar shaders
		ShaderProgram myFirstProgram;
		myFirstProgram.vertexShader = LoadVertexShader("MyFirstVertexShader.glsl");
		myFirstProgram.geometryShader = LoadGeometryShader("MyFirstGeometryShader.glsl");
		myFirstProgram.fragmentShader = LoadFragmentShader("MyFirstFragmentShader.glsl");

		//Cargo Modelo
		models.push_back(LoadOBJModel("Assets/Models/troll.obj"));

		//Compìlar programa
		compiledPrograms.push_back(CreateProgram(myFirstProgram));

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


		////////////////////////////////////////////  CUBO  ////////////////////////////////////////////

		//Declarar instancia de GameObject


		//Declarar vec2 para definir el offset
		glm::vec2 offset = glm::vec2(0.f, 0.f);

		//Compilar shaders
		ShaderProgram CuboProgram;
		CuboProgram.vertexShader = LoadVertexShader("VertexShaderCube.glsl");
		CuboProgram.geometryShader = LoadGeometryShader("GeometryShaderCube.glsl");
		CuboProgram.fragmentShader = LoadFragmentShader("FragmentShaderCube.glsl");

		//Compilar programa
		compiledPrograms.push_back(CreateProgram(CuboProgram));

		//Definimos color para limpiar el buffer de color
		glClearColor(0.f, 255.f, 255.f, 1.f);

		GLuint vaoCubo, vboCubo;

		//Definimos cantidad de vao a crear y donde almacenarlos 
		glGenVertexArrays(1, &vaoCubo);

		//Indico que el VAO activo de la GPU es el que acabo de crear
		glBindVertexArray(vaoCubo);

		//Definimos cantidad de vbo a crear y donde almacenarlos
		glGenBuffers(1, &vboCubo);

		//Indico que el VBO activo es el que acabo de crear y que almacenará un array. Todos los VBO que genere se asignaran al último VAO que he hecho glBindVertexArray
		glBindBuffer(GL_ARRAY_BUFFER, vboCubo);

		//Posición X e Y del punto
		GLfloat cubo[] = {
			-0.5f, -0.5f, -0.5f, // Bottom-left
	 0.5f, -0.5f, -0.5f, // Bottom-right
	 0.5f,  0.5f, -0.5f, // Top-right
	 0.5f,  0.5f, -0.5f, // Top-right
	-0.5f,  0.5f, -0.5f, // Top-left
	-0.5f, -0.5f, -0.5f, // Bottom-left

	-0.5f, -0.5f,  0.5f, // Bottom-left
	 0.5f, -0.5f,  0.5f, // Bottom-right
	 0.5f,  0.5f,  0.5f, // Top-right
	 0.5f,  0.5f,  0.5f, // Top-right
	-0.5f,  0.5f,  0.5f, // Top-left
	-0.5f, -0.5f,  0.5f, // Bottom-left

	-0.5f,  0.5f,  0.5f, // Top-left
	-0.5f,  0.5f, -0.5f, // Top-right
	-0.5f, -0.5f, -0.5f, // Bottom-right
	-0.5f, -0.5f, -0.5f, // Bottom-right
	-0.5f, -0.5f,  0.5f, // Bottom-left
	-0.5f,  0.5f,  0.5f, // Top-left

	 0.5f,  0.5f,  0.5f, // Top-left
	 0.5f,  0.5f, -0.5f, // Top-right
	 0.5f, -0.5f, -0.5f, // Bottom-right
	 0.5f, -0.5f, -0.5f, // Bottom-right
	 0.5f, -0.5f,  0.5f, // Bottom-left
	 0.5f,  0.5f,  0.5f, // Top-left

	-0.5f, -0.5f, -0.5f, // Bottom-left
	 0.5f, -0.5f, -0.5f, // Bottom-right
	 0.5f, -0.5f,  0.5f, // Top-right
	 0.5f, -0.5f,  0.5f, // Top-right
	-0.5f, -0.5f,  0.5f, // Top-left
	-0.5f, -0.5f, -0.5f, // Bottom-left

	-0.5f,  0.5f, -0.5f, // Bottom-left
	 0.5f,  0.5f, -0.5f, // Bottom-right
	 0.5f,  0.5f,  0.5f, // Top-right
	 0.5f,  0.5f,  0.5f, // Top-right
	-0.5f,  0.5f,  0.5f, // Top-left
	-0.5f,  0.5f, -0.5f  // Bottom-left
		};


		//Definimos modo de dibujo para cada cara
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//Ponemos los valores en el VBO creado
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubo), cubo, GL_STATIC_DRAW);

		//Indicamos donde almacenar y como esta distribuida la información
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

		//Indicamos que la tarjeta gráfica puede usar el atributo 0
		glEnableVertexAttribArray(0);

		//Desvinculamos VBO
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//Desvinculamos VAO
		glBindVertexArray(0);

		ShaderProgram WhiteProgram;
		WhiteProgram.vertexShader = LoadVertexShader("MyFirstVertexShader.glsl");
		WhiteProgram.geometryShader = LoadGeometryShader("MyFirstGeometryShader.glsl");
		WhiteProgram.fragmentShader = LoadFragmentShader("FragmentShaderBlanco.glsl");

		//Compìlar programa
		compiledPrograms.push_back(CreateProgram(WhiteProgram));

		ShaderProgram ColorProgram;
		ColorProgram.vertexShader = LoadVertexShader("MyFirstVertexShader.glsl");
		ColorProgram.geometryShader = LoadGeometryShader("MyFirstGeometryShader.glsl");
		ColorProgram.fragmentShader = LoadFragmentShader("FragmentShaderColor.glsl");

		//Compìlar programa
		compiledPrograms.push_back(CreateProgram(ColorProgram));

		//Generamos el game loop
		while (!glfwWindowShouldClose(window)) {

			//Pulleamos los eventos (botones, teclas, mouse...)
			glfwPollEvents();
			glfwSetKeyCallback(window, keyEvents);
			
			 if (detailPlane) 
			{
				 camara.position = glm::vec3(0.f, 1.5f, 0.f);
				 camara.direction = camara.position + glm::vec3(-1.5f, 0.65f, 0.f);
				 camara.fFov = 5.f;
			}
			 else if (dollyZoom) 
			 {
				 
				 if (first) 
				 {
					 camara.position = glm::vec3(0.f, 2.f, 5.f);
					 camara.direction = camara.position + glm::vec3(0.f, 0.f, -1.f);
					 camara.fFov = 45.f;
					 first = false;
				 }
				 else 
				 {
					 camara.initialPosition = glm::vec3(0.f, 2.f, 8.f);
					 camara.position.z -= camara.dollyZoomSpeed;
					 camara.fFov += camara.fovSpeed;
					 if (camara.position.z < camara.initialPosition.z - 4.0f) {  // Limitar el movimiento de la cámara
						 dollyZoom = false;  // Desactivar el dolly zoom después de alcanzar el límite
						 camara.position.z = camara.initialPosition.z;
						 camara.fFov = camara.initialFov;
						 first = true;
						 isometric = true;
					 }
				 }

				
			 }
			 else if (isometric)
			{
				 camara.position = glm::vec3(0.f, 2.f, 5.f);
				 camara.direction = camara.position + glm::vec3(0.f, 0.f, -1.f);
				 camara.fFov = 45.f;
					 first = false;
				
			 }
			 else if (generalPlane)
			 {
				 camara.position = glm::vec3(0.f, 1.5f, 0.f);
				 camara.direction = camara.position + glm::vec3(10.f, 0.5f, 1.f);
				 camara.fFov = 60.f;
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
			glm::mat4 view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

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
			models[0].Render();


			// <<<<<<<<<<<<<<<<<<<<<<<<<<TROLL 1>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

			
			glUseProgram(compiledPrograms[2]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glUniform1i(glGetUniformLocation(compiledPrograms[2], "textureSampler"), 0);

			//Definir la matriz de traslacion, rotacion y escalado
			 translationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(1.5f,1.f,0.f));
			 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-75.0f), glm::vec3(0.f, 1.f, 0.f));
			 scaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.5f));

			// Definir la matriz de vista
			 view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

			 // Definir la matriz proyeccion
			 projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);

			 

			//Asignar valores iniciales al programa
			glUniform2f(glGetUniformLocation(compiledPrograms[2], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);


			// Pasar las matrices
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(translationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			models[0].Render();


			// <<<<<<<<<<<<<<<<<<<<<<<<<<TROLL 2>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			glUseProgram(compiledPrograms[3]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glUniform1i(glGetUniformLocation(compiledPrograms[3], "textureSampler"), 0);

			//Definir la matriz de traslacion, rotacion y escalado
			translationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(-1.5f, 1.f, 0.f));
			rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(75.0f), glm::vec3(0.f, 1.f, 0.f));
			scaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.5f));

			// Definir la matriz de vista
			view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

			// Definir la matriz proyeccion
			projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);

			//Asignar valores iniciales al programa
			glUniform2f(glGetUniformLocation(compiledPrograms[3], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);


			// Pasar las matrices
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[3], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(translationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[3], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[3], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[3], "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[3], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			models[0].Render();



			//// <<<<<<<<<<<<<<<<<<<<<<<<<<PIEDRA ORIGINAL>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			glUseProgram(compiledPrograms[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, textureID2);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "textureSampler"), 1);

			//Definir la matriz de traslacion, rotacion y escalado
			glm::mat4 translationMatrix2 = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0.f));
			glm::mat4 rotationMatrix2 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.f, 0.f, 1.f));
			glm::mat4 scaleMatrix2 = glm::scale(glm::mat4(1.f), glm::vec3(0.5f));

			// Definir la matriz de vista
			view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

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
			models[1].Render();


			//// <<<<<<<<<<<<<<<<<<<<<<<<<<     NUBE   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
			glUseProgram(compiledPrograms[2]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, textureID2);
			glUniform1i(glGetUniformLocation(compiledPrograms[0], "textureSampler"), 1);

			//Definir la matriz de traslacion, rotacion y escalado
			 translationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 3.f, -5.f));
			 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.f, 0.f, 1.f));
			scaleMatrix = glm::scale(glm::mat4(1.f), glm::vec3(1.f,2.f,0.5f));

			view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

			// Definir la matriz proyeccion
			projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);
			//Asignar valores iniciales al programa
			glUniform2f(glGetUniformLocation(compiledPrograms[2], "windowSize"), WINDOW_WIDTH, WINDOW_HEIGHT);

			// Pasar las matrices
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "translationMatrix"), 1, GL_FALSE, glm::value_ptr(translationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "scaleMatrix"), 1, GL_FALSE, glm::value_ptr(scaleMatrix));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			models[1].Render();

			////////////////////////////////////////////  CUBO  ////////////////////////////////////////////			
			glUseProgram(compiledPrograms[1]);

			// Definir matrices de transformación
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, 1.5f, 3.0f));
			model = glm::scale(model, glm::vec3(10.f,1.f,10.f));

			view = glm::lookAt(camara.position, camara.direction, camara.localVectorUp);

			// Definir la matriz proyeccion
			projection = glm::perspective(glm::radians(camara.fFov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, camara.fNear, camara.fFar);

			// Pasar matrices a los shaders
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[1], "model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[1], "view"), 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(glGetUniformLocation(compiledPrograms[1], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

			// Dibujar el cubo
			glBindVertexArray(vaoCubo);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);

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