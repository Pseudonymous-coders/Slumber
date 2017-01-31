#pragma once
#ifndef SLUMBER_UI_H
#define SLUMBER_UI_H
#include <string>
#include <cstring>
#include <wchar.h>

namespace slumber {
	void setProgress(const unsigned int prog);

	void setStatus(const std::wstring& message);

	void setHumidity(const int humidity);

	void setTemperature(const int temp);

	void setMovement(const int move);

	void __loop_run();
	void runUI();
}

#endif//SLUMBER_UI_H
