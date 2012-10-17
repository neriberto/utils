#pragma once

#include <Windows.h>

class Gui
{
public:
	static Gui* Instance();
	static void DestroyInstance();

	bool NotifyFilterVerified(bool bVerified, const unsigned int uiTimeoutSecs = 5);
	bool NotifyURLFilterVerified(bool bVerified, const unsigned int uiTimeoutSecs = 5);

private:
	Gui();
	~Gui();

	static Gui* m_Instance;
	bool m_wasInitialized;

	bool init();
	void deinit();

	bool sendMessageToGui(const wchar_t *wcsMessage, unsigned int iTimeoutSecs = 5);
};