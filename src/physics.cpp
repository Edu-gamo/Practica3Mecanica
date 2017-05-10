#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <glm\gtc\matrix_transform.hpp>
#include <time.h>
#include <iostream>
#include <glm\gtc\quaternion.hpp>

using namespace glm;

bool show_test_window = false;

vec3 position = vec3(0.0f);
vec3 vel = vec3(0.0f);
vec3 postPos = vec3(0.0f);
vec3 postVel = vec3(0.0f);
vec3 linearMomentum;
vec3 angularMomentum;
//mat3 orientation;
quat orientation2;

float mass = 1.0f;

float eps = 0.5f;

vec3 J, Timpulse;

vec3 force = vec3(0.0f, -9.8f, 0.0f);

//Normales de los planos del eje X
float normalXRight[3] = { 0.f };
float normalXLeft[3] = { 0.f };

//Normales de los planos del eje Y
float normalYDown[3] = { 0.f };
float normalYTop[3] = { 0.f };

//Normales de los planos del eje Z
float normalZFront[3] = { 0.f };
float normalZBack[3] = { 0.f };

float dDown, dTop, dRight, dLeft, dFront, dBack;

void PhysicsInit();

namespace Cube {
	extern void setupCube();
	extern void cleanupCube();
	extern void updateCube(const glm::mat4& transform);
	extern void drawCube();
}

struct particle {
	vec3 position;
	vec3 vel = vec3(0.0f);
	vec3 postPos = vec3(0.0f);
	vec3 postVel = vec3(0.0f);

	float mass = 1.0f;
};

int numPart = 8;
particle *particles = new particle[8];

void GUI() {
	{	//FrameRate
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		//TODO
	}
	ImGui::Begin("");
	if (ImGui::Button("Resetear Simulacion")) { PhysicsInit(); }
	ImGui::End();

	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

void NormalPlane(vec3 pointA, vec3 pointB, vec3 pointC, float* normal) {

	float vectorA[3] = { pointA[0] - pointB[0], pointA[1] - pointB[1], pointA[2] - pointB[2] };
	float vectorB[3] = { pointC[0] - pointB[0], pointC[1] - pointB[1], pointC[2] - pointB[2] };

	normal[0] = vectorA[1] * vectorB[2] - vectorA[2] * vectorB[1];
	normal[1] = vectorA[2] * vectorB[0] - vectorA[0] * vectorB[2];
	normal[2] = vectorA[0] * vectorB[1] - vectorA[1] * vectorB[0];

	//Normalizar el vector
	float modulo = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
	normal[0] /= modulo;
	normal[1] /= modulo;
	normal[2] /= modulo;

}

void PhysicsInit() {
	//TODO
	srand(time(NULL));

	//Calcular la normal de los planos
	//Plano bajo
	vec3 pointA = vec3(-5.0f, 0.0f, -5.0f);
	vec3 pointB = vec3(-5.0f, 0.0f, 5.0f);
	vec3 pointC = vec3(5.0f, 0.0f, 5.0f);
	NormalPlane(pointA, pointB, pointC, normalYDown);
	dDown = -(normalYDown[0] * pointA.x) - (normalYDown[1] * pointA.y) - (normalYDown[2] * pointA.z);

	//Plano alto
	pointA = vec3(5.0f, 10.0f, 5.0f);
	pointB = vec3(-5.0f, 10.0f, 5.0f);
	pointC = vec3(-5.0f, 10.0f, -5.0f);
	NormalPlane(pointA, pointB, pointC, normalYTop);
	dTop = -(normalYTop[0] * pointA.x) - (normalYTop[1] * pointA.y) - (normalYTop[2] * pointA.z);

	//Plano derecha
	pointA = vec3(5.0f, 0.0f, -5.0f);
	pointB = vec3(5.0f, 0.0f, 5.0f);
	pointC = vec3(5.0f, 10.0f, 5.0f);
	NormalPlane(pointA, pointB, pointC, normalXRight);
	dRight = -(normalXRight[0] * pointA.x) - (normalXRight[1] * pointA.y) - (normalXRight[2] * pointA.z);

	//Plano izquierda
	pointA = vec3(-5.0f, 10.0f, 5.0f);
	pointB = vec3(-5.0f, 0.0f, 5.0f);
	pointC = vec3(-5.0f, 0.0f, -5.0f);
	NormalPlane(pointA, pointB, pointC, normalXLeft);
	dLeft = -(normalXLeft[0] * pointA.x) - (normalXLeft[1] * pointA.y) - (normalXLeft[2] * pointA.z);

	//Plano frontal
	pointA = vec3(5.0f, 0.0f, 5.0f);
	pointB = vec3(-5.0f, 0.0f, 5.0f);
	pointC = vec3(-5.0f, 10.0f, 5.0f);
	NormalPlane(pointA, pointB, pointC, normalZFront);
	dFront = -(normalZFront[0] * pointA.x) - (normalZFront[1] * pointA.y) - (normalZFront[2] * pointA.z);

	//Plano trasero
	pointA = vec3(-5.0f, 10.0f, -5.0f);
	pointB = vec3(-5.0f, 0.0f, -5.0f);
	pointC = vec3(5.0f, 0.0f, -5.0f);
	NormalPlane(pointA, pointB, pointC, normalZBack);
	dBack = -(normalZBack[0] * pointA.x) - (normalZBack[1] * pointA.y) - (normalZBack[2] * pointA.z);
	
	position = vec3(0.0f, 7.0f, 0.0f);

	particles[0].position = vec3(position.x - 0.5f, position.y - 0.5f, position.z - 0.5f);
	particles[1].position = vec3(position.x - 0.5f, position.y - 0.5f, position.z + 0.5f);
	particles[2].position = vec3(position.x + 0.5f, position.y - 0.5f, position.z - 0.5f);
	particles[3].position = vec3(position.x + 0.5f, position.y - 0.5f, position.z + 0.5f);
	particles[4].position = vec3(position.x - 0.5f, position.y + 0.5f, position.z - 0.5f);
	particles[5].position = vec3(position.x - 0.5f, position.y + 0.5f, position.z + 0.5f);
	particles[6].position = vec3(position.x + 0.5f, position.y + 0.5f, position.z - 0.5f);
	particles[7].position = vec3(position.x + 0.5f, position.y + 0.5f, position.z + 0.5f);

	vec3 torque, torqueForce;
	torqueForce = vec3(rand() % 5, rand() % 5, rand() % 5);
	torque = cross(particles[rand() % 8].position - position, torqueForce);
	angularMomentum = angularMomentum + 0.033f * torque;

	linearMomentum = vec3(rand() % 10 - 5, rand() % 10 - 5, rand() % 10 - 5);

}

void PhysicsUpdate(float dt) {
	//TODO
	dt /= 100;

	for (int i = 0; i < 100; i++) {

		//Calcular velocidad
		postVel.x = vel.x + dt * (force.x / mass);
		postVel.y = vel.y + dt * (force.y / mass);
		postVel.z = vel.z + dt * (force.z / mass);

		linearMomentum = linearMomentum + dt * force;
		vec3 v = linearMomentum / mass;
		postPos = position + dt * v;
		mat3 I = mat3(1.0f / 12.0f * mass*(pow(1, 2) + pow(1, 2)));
		mat3 orientation = glm::mat3_cast(orientation2);
		mat3 I2 = orientation * inverse(I) * transpose(orientation);
		vec3 w = I2 * angularMomentum;
		//mat3 wMat = mat3(vec3(0, w.z, -w.y), vec3(-w.z, 0, w.x), vec3(w.y, -w.x, 0));
		//orientation = orientation + dt * (wMat * orientation);
		orientation2 = orientation2 + dt * (0.5f*quat(0, w) * orientation2);
		orientation2 = normalize(orientation2);

		for (int i = 0; i < 8; i++) {
			particles[i].vel = vel;
			particles[i].postVel = postVel;
		}

		particles[0].position = vec3(position.x - 0.5f, position.y - 0.5f, position.z - 0.5f);
		particles[1].position = vec3(position.x - 0.5f, position.y - 0.5f, position.z + 0.5f);
		particles[2].position = vec3(position.x + 0.5f, position.y - 0.5f, position.z - 0.5f);
		particles[3].position = vec3(position.x + 0.5f, position.y - 0.5f, position.z + 0.5f);
		particles[4].position = vec3(position.x - 0.5f, position.y + 0.5f, position.z - 0.5f);
		particles[5].position = vec3(position.x - 0.5f, position.y + 0.5f, position.z + 0.5f);
		particles[6].position = vec3(position.x + 0.5f, position.y + 0.5f, position.z - 0.5f);
		particles[7].position = vec3(position.x + 0.5f, position.y + 0.5f, position.z + 0.5f);

		particles[0].postPos = vec3(postPos.x - 0.5f, postPos.y - 0.5f, postPos.z - 0.5f);
		particles[1].postPos = vec3(postPos.x - 0.5f, postPos.y - 0.5f, postPos.z + 0.5f);
		particles[2].postPos = vec3(postPos.x + 0.5f, postPos.y - 0.5f, postPos.z - 0.5f);
		particles[3].postPos = vec3(postPos.x + 0.5f, postPos.y - 0.5f, postPos.z + 0.5f);
		particles[4].postPos = vec3(postPos.x - 0.5f, postPos.y + 0.5f, postPos.z - 0.5f);
		particles[5].postPos = vec3(postPos.x - 0.5f, postPos.y + 0.5f, postPos.z + 0.5f);
		particles[6].postPos = vec3(postPos.x + 0.5f, postPos.y + 0.5f, postPos.z - 0.5f);
		particles[7].postPos = vec3(postPos.x + 0.5f, postPos.y + 0.5f, postPos.z + 0.5f);

		for (int i = 0; i < 8; i++) {
			//Detectar Colisiones
			float dotProductDown = (normalYDown[0] * particles[i].position.x + normalYDown[1] * particles[i].position.y + normalYDown[2] * particles[i].position.z);
			float dotProductPostDown = (normalYDown[0] * particles[i].postPos.x + normalYDown[1] * particles[i].postPos.y + normalYDown[2] * particles[i].postPos.z);

			//Plano bajo
			if ((dotProductDown + dDown) * (dotProductPostDown + dDown) <= 0) {

				vec3 p = particles[i].vel + cross(w, (particles[i].position - position));
				vec3 vRel = vec3(normalYDown[0], normalYDown[1], normalYDown[2]) * p;
				vec3 j = (-(1 + eps)*vRel) / (1 / mass + vec3(normalYDown[0], normalYDown[1], normalYDown[2]) * cross(inverse(I) * cross(particles[i].position, vec3(normalYDown[0], normalYDown[1], normalYDown[2])), particles[i].position));

				J = j*vec3(normalYDown[0], normalYDown[1], normalYDown[2]);
				Timpulse = cross(particles[i].position, J);

				linearMomentum += J;
				angularMomentum += Timpulse;

			}

			float dotProductTop = (normalYTop[0] * particles[i].position.x + normalYTop[1] * particles[i].position.y + normalYTop[2] * particles[i].position.z);
			float dotProductPostTop = (normalYTop[0] * particles[i].postPos.x + normalYTop[1] * particles[i].postPos.y + normalYTop[2] * particles[i].postPos.z);

			//Plano alto
			if ((dotProductTop + dTop) * (dotProductPostTop + dTop) <= 0) {

				vec3 p = particles[i].vel + cross(w, (particles[i].position - position));
				vec3 vRel = vec3(normalYTop[0], normalYTop[1], normalYTop[2]) * p;
				vec3 j = (-(1 + eps)*vRel) / (1 / mass + vec3(normalYTop[0], normalYTop[1], normalYTop[2]) * cross(inverse(I) * cross(particles[i].position, vec3(normalYTop[0], normalYTop[1], normalYTop[2])), particles[i].position));

				J = j*vec3(normalYTop[0], normalYTop[1], normalYTop[2]);
				Timpulse = cross(particles[i].position, J);

				linearMomentum += J;
				angularMomentum += Timpulse;

			}

			float dotProductRight = (normalXRight[0] * particles[i].position.x + normalXRight[1] * particles[i].position.y + normalXRight[2] * particles[i].position.z);
			float dotProductPostRight = (normalXRight[0] * particles[i].postPos.x + normalXRight[1] * particles[i].postPos.y + normalXRight[2] * particles[i].postPos.z);

			//Plano darecha
			if ((dotProductRight + dRight) * (dotProductPostRight + dRight) <= 0) {

				vec3 p = particles[i].vel + cross(w, (particles[i].position - position));
				vec3 vRel = vec3(normalXRight[0], normalXRight[1], normalXRight[2]) * p;
				vec3 j = (-(1 + eps)*vRel) / (1 / mass + vec3(normalXRight[0], normalXRight[1], normalXRight[2]) * cross(inverse(I) * cross(particles[i].position, vec3(normalXRight[0], normalXRight[1], normalXRight[2])), particles[i].position));

				J = j*vec3(normalXRight[0], normalXRight[1], normalXRight[2]);
				Timpulse = cross(particles[i].position, J);

				linearMomentum += J;
				angularMomentum += Timpulse;
			}

			float dotProductLeft = (normalXLeft[0] * particles[i].position.x + normalXLeft[1] * particles[i].position.y + normalXLeft[2] * particles[i].position.z);
			float dotProductPostLeft = (normalXLeft[0] * particles[i].postPos.x + normalXLeft[1] * particles[i].postPos.y + normalXLeft[2] * particles[i].postPos.z);

			//Plano izquierda
			if ((dotProductLeft + dLeft) * (dotProductPostLeft + dLeft) <= 0) {

				vec3 p = particles[i].vel + cross(w, (particles[i].position - position));
				vec3 vRel = vec3(normalXLeft[0], normalXLeft[1], normalXLeft[2]) * p;
				vec3 j = (-(1 + eps)*vRel) / (1 / mass + vec3(normalXLeft[0], normalXLeft[1], normalXLeft[2]) * cross(inverse(I) * cross(particles[i].position, vec3(normalXLeft[0], normalXLeft[1], normalXLeft[2])), particles[i].position));

				J = j*vec3(normalXLeft[0], normalXLeft[1], normalXLeft[2]);
				Timpulse = cross(particles[i].position, J);

				linearMomentum += J;
				angularMomentum += Timpulse;
			}

			float dotProductFront = (normalZFront[0] * particles[i].position.x + normalZFront[1] * particles[i].position.y + normalZFront[2] * particles[i].position.z);
			float dotProductPostFront = (normalZFront[0] * particles[i].postPos.x + normalZFront[1] * particles[i].postPos.y + normalZFront[2] * particles[i].postPos.z);

			//Plano frontal
			if ((dotProductFront + dFront) * (dotProductPostFront + dFront) <= 0) {

				vec3 p = particles[i].vel + cross(w, (particles[i].position - position));
				vec3 vRel = vec3(normalZFront[0], normalZFront[1], normalZFront[2]) * p;
				vec3 j = (-(1 + eps)*vRel) / (1 / mass + vec3(normalZFront[0], normalZFront[1], normalZFront[2]) * cross(inverse(I) * cross(particles[i].position, vec3(normalZFront[0], normalZFront[1], normalZFront[2])), particles[i].position));

				J = j*vec3(normalZFront[0], normalZFront[1], normalZFront[2]);
				Timpulse = cross(particles[i].position, J);

				linearMomentum += J;
				angularMomentum += Timpulse;
			}

			float dotProductBack = (normalZBack[0] * particles[i].position.x + normalZBack[1] * particles[i].position.y + normalZBack[2] * particles[i].position.z);
			float dotProductPostBack = (normalZBack[0] * particles[i].postPos.x + normalZBack[1] * particles[i].postPos.y + normalZBack[2] * particles[i].postPos.z);

			//Plano trasero
			if ((dotProductBack + dBack) * (dotProductPostBack + dBack) <= 0) {

				vec3 p = particles[i].vel + cross(w, (particles[i].position - position));
				vec3 vRel = vec3(normalZBack[0], normalZBack[1], normalZBack[2]) * p;
				vec3 j = (-(1 + eps)*vRel) / (1 / mass + vec3(normalZBack[0], normalZBack[1], normalZBack[2]) * cross(inverse(I) * cross(particles[i].position, vec3(normalZBack[0], normalZBack[1], normalZBack[2])), particles[i].position));

				J = j*vec3(normalZBack[0], normalZBack[1], normalZBack[2]);
				Timpulse = cross(particles[i].position, J);

				linearMomentum += J;
				angularMomentum += Timpulse;
			}
		}

		/*//Detectar Colisiones
		float dotProductDown = (normalYDown[0] * position.x + normalYDown[1] * position.y + normalYDown[2] * position.z);
		float dotProductPostDown = (normalYDown[0] * postPos.x + normalYDown[1] * postPos.y + normalYDown[2] * postPos.z);

		//Plano bajo
		if ((dotProductDown + dDown) * (dotProductPostDown + dDown) <= 0) {

			float dotProductPostVelDown = (normalYDown[0] * postVel.x + normalYDown[1] * postVel.y + normalYDown[2] * postVel.z);
			float dotProductVelDown = (normalYDown[0] * vel.x + normalYDown[1] * vel.y + normalYDown[2] * vel.z);
			float normalVel[3] = { dotProductVelDown * normalYDown[0], dotProductVelDown * normalYDown[1], dotProductVelDown * normalYDown[2] };
			float tangVel[3] = { vel.x - normalVel[0], vel.y - normalVel[1], vel.z - normalVel[2] };

			postPos.x = postPos.x - 1.1 * (dotProductPostDown + dDown)*normalYDown[0];
			postPos.y = postPos.y - 1.1 * (dotProductPostDown + dDown)*normalYDown[1];
			postPos.z = postPos.z - 1.1 * (dotProductPostDown + dDown)*normalYDown[2];

			postVel.x = postVel.x - 1.1 * (dotProductPostVelDown)*normalYDown[0];
			postVel.y = postVel.y - 1.1 * (dotProductPostVelDown)*normalYDown[1];
			postVel.z = postVel.z - 1.1 * (dotProductPostVelDown)*normalYDown[2];

			postVel.x = postVel.x - 1 * tangVel[0];
			postVel.y = postVel.y - 1 * tangVel[1];
			postVel.z = postVel.z - 1 * tangVel[2];
		}

		float dotProductTop = (normalYTop[0] * position.x + normalYTop[1] * position.y + normalYTop[2] * position.z);
		float dotProductPostTop = (normalYTop[0] * postPos.x + normalYTop[1] * postPos.y + normalYTop[2] * postPos.z);

		//Plano alto
		if ((dotProductTop + dTop) * (dotProductPostTop + dTop) <= 0) {

			float dotProductPostVelTop = (normalYTop[0] * postVel.x + normalYTop[1] * postVel.y + normalYTop[2] * postVel.z);
			float dotProductVelTop = (normalYTop[0] * vel.x + normalYTop[1] * vel.y + normalYTop[2] * vel.z);
			float normalVel[3] = { dotProductVelTop * normalYTop[0], dotProductVelTop * normalYTop[1], dotProductVelTop * normalYTop[2] };
			float tangVel[3] = { vel.x - normalVel[0], vel.y - normalVel[1], vel.z - normalVel[2] };

			postPos.x = postPos.x - 1.1 * (dotProductPostTop + dTop)*normalYTop[0];
			postPos.y = postPos.y - 1.1 * (dotProductPostTop + dTop)*normalYTop[1];
			postPos.z = postPos.z - 1.1 * (dotProductPostTop + dTop)*normalYTop[2];

			postVel.x = postVel.x - 1.1 * (dotProductPostVelTop)*normalYTop[0];
			postVel.y = postVel.y - 1.1 * (dotProductPostVelTop)*normalYTop[1];
			postVel.z = postVel.z - 1.1 * (dotProductPostVelTop)*normalYTop[2];

			postVel.x = postVel.x - 1 * tangVel[0];
			postVel.y = postVel.y - 1 * tangVel[1];
			postVel.z = postVel.z - 1 * tangVel[2];
		}

		float dotProductRight = (normalXRight[0] * position.x + normalXRight[1] * position.y + normalXRight[2] * position.z);
		float dotProductPostRight = (normalXRight[0] * postPos.x + normalXRight[1] * postPos.y + normalXRight[2] * postPos.z);

		//Plano darecha
		if ((dotProductRight + dRight) * (dotProductPostRight + dRight) <= 0) {

			float dotProductPostVelRight = (normalXRight[0] * postVel.x + normalXRight[1] * postVel.y + normalXRight[2] * postVel.z);
			float dotProductVelRight = (normalXRight[0] * vel.x + normalXRight[1] * vel.y + normalXRight[2] * vel.z);
			float normalVel[3] = { dotProductVelRight * normalXRight[0], dotProductVelRight * normalXRight[1], dotProductVelRight * normalXRight[2] };
			float tangVel[3] = { vel.x - normalVel[0], vel.y - normalVel[1], vel.z - normalVel[2] };

			postPos.x = postPos.x - 1.1 * (dotProductPostRight + dRight)*normalXRight[0];
			postPos.y = postPos.y - 1.1 * (dotProductPostRight + dRight)*normalXRight[1];
			postPos.z = postPos.z - 1.1 * (dotProductPostRight + dRight)*normalXRight[2];

			postVel.x = postVel.x - 1.1 * (dotProductPostVelRight)*normalXRight[0];
			postVel.y = postVel.y - 1.1 * (dotProductPostVelRight)*normalXRight[1];
			postVel.z = postVel.z - 1.1 * (dotProductPostVelRight)*normalXRight[2];

			postVel.x = postVel.x - 1 * tangVel[0];
			postVel.y = postVel.y - 1 * tangVel[1];
			postVel.z = postVel.z - 1 * tangVel[2];
		}

		float dotProductLeft = (normalXLeft[0] * position.x + normalXLeft[1] * position.y + normalXLeft[2] * position.z);
		float dotProductPostLeft = (normalXLeft[0] * postPos.x + normalXLeft[1] * postPos.y + normalXLeft[2] * postPos.z);

		//Plano izquierda
		if ((dotProductLeft + dLeft) * (dotProductPostLeft + dLeft) <= 0) {

			float dotProductPostVelLeft = (normalXLeft[0] * postVel.x + normalXLeft[1] * postVel.y + normalXLeft[2] * postVel.z);
			float dotProductVelLeft = (normalXLeft[0] * vel.x + normalXLeft[1] * vel.y + normalXLeft[2] * vel.z);
			float normalVel[3] = { dotProductVelLeft * normalXLeft[0], dotProductVelLeft * normalXLeft[1], dotProductVelLeft * normalXLeft[2] };
			float tangVel[3] = { vel.x - normalVel[0], vel.y - normalVel[1], vel.z - normalVel[2] };

			postPos.x = postPos.x - 1.1 * (dotProductPostLeft + dLeft)*normalXLeft[0];
			postPos.y = postPos.y - 1.1 * (dotProductPostLeft + dLeft)*normalXLeft[1];
			postPos.z = postPos.z - 1.1 * (dotProductPostLeft + dLeft)*normalXLeft[2];

			postVel.x = postVel.x - 1.1 * (dotProductPostVelLeft)*normalXLeft[0];
			postVel.y = postVel.y - 1.1 * (dotProductPostVelLeft)*normalXLeft[1];
			postVel.z = postVel.z - 1.1 * (dotProductPostVelLeft)*normalXLeft[2];

			postVel.x = postVel.x - 1 * tangVel[0];
			postVel.y = postVel.y - 1 * tangVel[1];
			postVel.z = postVel.z - 1 * tangVel[2];
		}

		float dotProductFront = (normalZFront[0] * position.x + normalZFront[1] * position.y + normalZFront[2] * position.z);
		float dotProductPostFront = (normalZFront[0] * postPos.x + normalZFront[1] * postPos.y + normalZFront[2] * postPos.z);

		//Plano frontal
		if ((dotProductFront + dFront) * (dotProductPostFront + dFront) <= 0) {

			float dotProductPostVelFront = (normalZFront[0] * postVel.x + normalZFront[1] * postVel.y + normalZFront[2] * postVel.z);
			float dotProductVelFront = (normalZFront[0] * vel.x + normalZFront[1] * vel.y + normalZFront[2] * vel.z);
			float normalVel[3] = { dotProductVelFront * normalZFront[0], dotProductVelFront * normalZFront[1], dotProductVelFront * normalZFront[2] };
			float tangVel[3] = { vel.x - normalVel[0], vel.y - normalVel[1], vel.z - normalVel[2] };

			postPos.x = postPos.x - 1.1 * (dotProductPostFront + dFront)*normalZFront[0];
			postPos.y = postPos.y - 1.1 * (dotProductPostFront + dFront)*normalZFront[1];
			postPos.z = postPos.z - 1.1 * (dotProductPostFront + dFront)*normalZFront[2];

			postVel.x = postVel.x - 1.1 * (dotProductPostVelFront)*normalZFront[0];
			postVel.y = postVel.y - 1.1 * (dotProductPostVelFront)*normalZFront[1];
			postVel.z = postVel.z - 1.1 * (dotProductPostVelFront)*normalZFront[2];

			postVel.x = postVel.x - 1 * tangVel[0];
			postVel.y = postVel.y - 1 * tangVel[1];
			postVel.z = postVel.z - 1 * tangVel[2];
		}

		float dotProductBack = (normalZBack[0] * position.x + normalZBack[1] * position.y + normalZBack[2] * position.z);
		float dotProductPostBack = (normalZBack[0] * postPos.x + normalZBack[1] * postPos.y + normalZBack[2] * postPos.z);

		//Plano trasero
		if ((dotProductBack + dBack) * (dotProductPostBack + dBack) <= 0) {

			float dotProductPostVelBack = (normalZBack[0] * postVel.x + normalZBack[1] * postVel.y + normalZBack[2] * postVel.z);
			float dotProductVelBack = (normalZBack[0] * vel.x + normalZBack[1] * vel.y + normalZBack[2] * vel.z);
			float normalVel[3] = { dotProductVelBack * normalZBack[0], dotProductVelBack * normalZBack[1], dotProductVelBack * normalZBack[2] };
			float tangVel[3] = { vel.x - normalVel[0], vel.y - normalVel[1], vel.z - normalVel[2] };

			postPos.x = postPos.x - 1.1 * (dotProductPostBack + dBack)*normalZBack[0];
			postPos.y = postPos.y - 1.1 * (dotProductPostBack + dBack)*normalZBack[1];
			postPos.z = postPos.z - 1.1 * (dotProductPostBack + dBack)*normalZBack[2];

			postVel.x = postVel.x - 1.1 * (dotProductPostVelBack)*normalZBack[0];
			postVel.y = postVel.y - 1.1 * (dotProductPostVelBack)*normalZBack[1];
			postVel.z = postVel.z - 1.1 * (dotProductPostVelBack)*normalZBack[2];

			postVel.x = postVel.x - 1 * tangVel[0];
			postVel.y = postVel.y - 1 * tangVel[1];
			postVel.z = postVel.z - 1 * tangVel[2];
		}*/

		position = postPos;
		vel = postVel;

	}

	glm::mat4 transform, translation, rotation;
	translation = translate(translation, position);
	rotation = mat4_cast(orientation2);
	transform = translation * rotation;

	Cube::updateCube(transform);
}

void PhysicsCleanup() {
	//TODO
}