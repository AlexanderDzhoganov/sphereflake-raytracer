#ifndef __CAMERA_H
#define __CAMERA_H

class Camera
{

public:
	mat4 getViewMatrix() const
	{
		mat4 translate = mat4(1.0);
		translate[3] = vec4(m_Position, 1.0);
		mat4 rotate = glm::toMat4(getOrientation());
		return inverse(translate * rotate);
	}

	mat4 getProjectionMatrix() const
	{
		return glm::perspective(radians(m_Fov), m_AspectRatio, 0.01f, 100.0f);
	}

	mat4 getViewProjectionMatrix() const
	{
		return getViewMatrix() * getProjectionMatrix();
	}

	vec3 getTopLeft() const
	{
		auto d = getScaling();
		return m_Position + getOrientation() * vec3(-m_AspectRatio * d, d, -1.0f);
	}

	vec3 getTopRight() const
	{
		auto d = getScaling();
		return m_Position + getOrientation() * vec3(m_AspectRatio * d, d, -1.0f);
	}

	vec3 getBottomLeft() const
	{
		auto d = getScaling();
		return m_Position + getOrientation() * vec3(-m_AspectRatio * d, -d, -1.0f);
	}

	const vec3& getPosition() const
	{
		return m_Position;
	}

	void setPosition(const vec3& position)
	{
		m_Position = position;
	}

	quat getOrientation() const
	{
		return quat(vec3(m_Yaw, m_Pitch, m_Roll));
	}

	float getRoll() const
	{
		return m_Roll;
	}

	void setRoll(float roll)
	{
		m_Roll = roll;
	}

	float getPitch() const
	{
		return m_Pitch;
	}

	void setPitch(float pitch)
	{
		m_Pitch = pitch;
	}

	float getYaw() const
	{
		return m_Yaw;
	}

	void setYaw(float yaw)
	{
		m_Yaw = yaw;
	}

	float getZoom() const
	{
		return m_Zoom;
	}

	void setZoom(float zoom)
	{
		m_Zoom = zoom;
	}

	float getFov() const
	{
		return m_Fov;
	}

	void setFov(float fov)
	{
		m_Fov = fov;
	}

	private:
	float getScaling() const
	{
		return tanf(glm::radians(m_Fov / 2.0f)) / vec3(-m_AspectRatio, 1.0, 0.0).length();
	}

	vec3 m_Position;
	float m_Fov = 60.0f;
	float m_AspectRatio = 16.0f / 9.0f;
	float m_Roll = 0.0;
	float m_Pitch = 0.0;
	float m_Yaw = 0.0;
	float m_Zoom = 0.1;

};

#endif // __CAMERA_H