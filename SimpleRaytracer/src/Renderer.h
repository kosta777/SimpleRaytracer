#pragma once

#include "Walnut/Image.h"

#include <memory>
#include <glm/glm.hpp>
#include "Ray.h"
#include "Camera.h"
#include "Scene.h"

class Renderer
{
public:
	Renderer() = default;

	void OnResize(uint32_t width, uint32_t height);
	void Render(const Scene& scene, const Camera&);

	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }
private:
	struct RayHitPayload {
		bool isHit = false;
		float hitDistance;
		glm::vec3 WorlPosition;
		glm::vec3 WorldNormal;

		uint32_t ObjectIndex;
	};

	//here we are passing x and y to say which shader it is, on GPU its passed automatically to the shader
	glm::vec4 RayGen(uint32_t x, uint32_t y); //called per pixel

	//Will shoot a ray out to a particilar point and will return info on what happened
	RayHitPayload TraceRay(const Ray& ray);
	RayHitPayload ClosestHit(const Ray& ray, float hitDistancem, uint32_t objectIndex);
	RayHitPayload Miss(const Ray& ray);

	//AnyHitShader missing (for translucent stuff)
private:
		std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;
};