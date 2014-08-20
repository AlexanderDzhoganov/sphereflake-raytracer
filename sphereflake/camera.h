#ifndef __CAMERA_H
#define __CAMERA_H

namespace SphereflakeRaytracer
{

	class Camera
	{

		public:
		mat4 GetViewMatrix() const
		{
			mat4 translate = mat4(1.0);
			translate[3] = vec4(m_Position, 1.0);
			mat4 rotate = glm::toMat4(GetOrientation());
			return inverse(translate * rotate);
		}

		mat4 GetProjectionMatrix() const
		{
			return glm::perspective(radians(m_FOV), m_Aspect, 0.01f, 100.0f);
		}

		mat4 GetViewProjectionMatrix() const
		{
			return GetViewMatrix() * GetProjectionMatrix();
		}

		vec3 GetTopLeft() const
		{
			auto d = GetScaling();
			return m_Position + GetOrientation() * vec3(-m_Aspect * d, d, -1.0f);
		}

		vec3 GetTopRight() const
		{
			auto d = GetScaling();
			return m_Position + GetOrientation() * vec3(m_Aspect * d, d, -1.0f);
		}

		vec3 GetBottomLeft() const
		{
			auto d = GetScaling();
			return m_Position + GetOrientation() * vec3(-m_Aspect * d, -d, -1.0f);
		}

		const vec3& GetPosition() const
		{
			return m_Position;
		}

		void SetPosition(const vec3& position)
		{
			m_Position = position;
		}

		quat GetOrientation() const
		{
			return quat(vec3(m_Yaw, m_Pitch, m_Roll));
		}

		float GetRoll() const
		{
			return m_Roll;
		}

		void SetRoll(float roll)
		{
			m_Roll = roll;
		}

		float GetPitch() const
		{
			return m_Pitch;
		}

		void SetPitch(float pitch)
		{
			m_Pitch = pitch;
		}

		float GetYaw() const
		{
			return m_Yaw;
		}

		void SetYaw(float yaw)
		{
			m_Yaw = yaw;
		}

		float GetFOV() const
		{
			return m_FOV;
		}

		void SetFOV(float fov)
		{
			m_FOV = fov;
		}

		private:
		float GetScaling() const
		{
			return tanf(glm::radians(m_FOV / 2.0f)) / vec3(-m_Aspect, 1.0, 0.0).length();
		}

		vec3 m_Position;
		float m_FOV = 60.0f;
		float m_Aspect = 16.0f / 9.0f;
		float m_Roll = 0.0;
		float m_Pitch = 0.0;
		float m_Yaw = 0.0;

	};

}

#endif // __CAMERA_H