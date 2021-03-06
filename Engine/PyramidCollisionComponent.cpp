//
// Bachelor of Software Engineering
// Media Design School
// Auckland
// New Zealand
//
// (c) 2018 Media Design School
//
// Author       : Lance Chaney
// Mail         : lance.cha7337@mediadesign.school.nz
//

#include "PyramidCollisionComponent.h"
#include "GLMUtils.h"
#include "GLPrimitives.h"
#include "VertexFormat.h"

std::vector<glm::vec3> PyramidCollisionComponent::GetPoints(TransformComponent& pyramidTransform)
{
	std::vector<glm::vec3> transformedPoints;
	glm::mat4 objectMatrix = GLMUtils::transformToMat(pyramidTransform);
	const std::vector<VertexFormat>& pyramidVertices = GLPrimitives::getPyramidVertices();

	for (const VertexFormat& vertex : pyramidVertices) {
		transformedPoints.push_back(objectMatrix * glm::vec4(vertex.position,1.0f));
	}
	return transformedPoints;
}
