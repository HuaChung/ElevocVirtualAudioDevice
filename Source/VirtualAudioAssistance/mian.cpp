#include "CVirtualAudioAssistance.h"
#include <memory>

int main(int argc, char* argv[])
{
	SetDllDirectory(L".\\Lib");
	std::shared_ptr<CVirtualAudioAssistance> pVirtualAudioAssistance(new CVirtualAudioAssistance());
	pVirtualAudioAssistance->Run();
	system("pause");
	return 0;
}