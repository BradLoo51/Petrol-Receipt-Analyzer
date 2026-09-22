#include <wx/dnd.h>

class MainFrame;

class FileDropDown : public wxFileDropTarget
{
public:
    FileDropDown(MainFrame* frame);

    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;
private:
    MainFrame* parentFrame;
};

