#pragma once
#include <list>
#include <functional>

#include <include/dll_export.h>
#include <Event/EventListener.h>

class DLLExport SimpleTimer
	: public EventListener
{
	public:
		SimpleTimer(float duration);
		~SimpleTimer();

		void Start();
		void Reset();
		void Stop();
		void Update();
		bool IsActive();
		void SetDuration(float duration);
		void OnExpire(std::function<void()> func);

	private:
		void OnEvent(EventType Event, void *data);

	private:
		bool isActive;
		float duration;
		float startTime;
		std::list<std::function<void()>> onExprire;
};