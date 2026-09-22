#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(App);

bool App::OnInit()
{
	wxInitAllImageHandlers();

	MainFrame* mainFrame = new MainFrame("Petrol Receipt Analyzer");
	mainFrame->SetClientSize(800, 600);
	mainFrame->Center();
	mainFrame->Show();
	return true;
}
