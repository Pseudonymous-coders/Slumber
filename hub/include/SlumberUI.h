#pragma once
#ifndef SLUMBER_UI_H
#define SLUMBER_UI_H
#include <string>

namespace slumber {
	void setProgress(const unsigned int prog);

	void setStatus(const std::wstring& message);

	void setHumidity(const int humidity);

	void setTemperature(const int temp);

	void setMovement(const int move);

	void runUI();
}

#endif//SLUMBER_UI_H