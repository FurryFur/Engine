#include "CapsuleCollisionComponent.h"
#include "GLMUtils.h"

CapsuleCollisionComponent::CapsuleCollisionComponent()
	: radius(1.0f)
	, capsuleEnd{0.0f, 1.0f, 0.0f}
{
}

glm::vec3 CapsuleCollisionComponent::getCapsuleEnd(TransformComponent & capsuleTransform)
{
	glm::mat4 objectMatrix = GLMUtils::transformToMat(capsuleTransform);

	return objectMatrix * glm::vec4(capsuleEnd, 1.0f);
}

// works as long as it is scaled uniformly
float CapsuleCollisionComponent::getRadius(TransformComponent & capsuleTransform)
{
	return radius * capsuleTransform.scale.y;
}
