#pragma once
#ifndef SLUMBER_UI_H
#define SLUMBER_UI_H
#include <string>

void setProgress(const unsigned int prog);

void setStatus(const std::wstring& message);
void setStatus(const std::string& message);

#endif//SLUMBER_UI_H