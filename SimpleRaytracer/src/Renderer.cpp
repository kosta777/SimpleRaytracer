#include "Renderer.h"

#include "Walnut/Random.h"

namespace Utils {

	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}

}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		// No resize necessary
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;
	
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec4 color = RayGen(x, y);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));

			//in GPU this is done in the RayGen shader and not here
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::RayGen(uint32_t x, uint32_t y) {
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	int bounces = 2;
	float bounceMultiplier = 1.0f;
	glm::vec3 color(0.0f);

	for (int i = 0; i < bounces; i++) {
		auto payload = TraceRay(ray);
		if (!payload.isHit) {
			glm::vec3 skyColor = glm::vec3(0.0f, 0.0f, 0.0f);
			color += skyColor * bounceMultiplier;
			break;
			//return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}

		glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));
		float lightIntensity = glm::max(glm::dot(payload.WorldNormal, -lightDir), 0.0f); // == cos(angle)
		glm::vec3 sphereColor = m_ActiveScene->Spheres[payload.ObjectIndex].Albedo;
		sphereColor *= lightIntensity;
		color += sphereColor * bounceMultiplier;

		bounceMultiplier *= 0.7f;

		//moving the new origin a bit forward along the normal for the new ray, since if we don't, the ray could hit the origin object before leaving it
		ray.Origin = payload.WorlPosition + payload.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal);
	}

	return glm::vec4(color, 1.0f);
}

Renderer::RayHitPayload Renderer::TraceRay(const Ray& ray)
{
	// (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
	// where
	// a = ray origin
	// b = ray direction
	// r = radius
	// t = hit distance

	uint32_t closestSphere = UINT32_MAX;
	float hitDistance = std::numeric_limits<float>::max();

	for (size_t i=0; i< m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Quadratic forumula discriminant:
		// b^2 - 4ac

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
			continue;

		// Quadratic formula:
		// (-b +- sqrt(discriminant)) / 2a

		// float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT> 0 && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = i;
		}
	}

	if (closestSphere == UINT32_MAX)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphere);

	}

Renderer::RayHitPayload Renderer::Miss(const Ray& ray) {
	return RayHitPayload();
}

Renderer::RayHitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, uint32_t objectIndex) {
	RayHitPayload payload;
	payload.isHit = true;
	payload.hitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorlPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorlPosition);

	//Since the way we calculate origin included moving the sphere to us, now we need to undo it
	payload.WorlPosition += closestSphere.Position;
	
	return payload;
}
