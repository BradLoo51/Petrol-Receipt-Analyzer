#include "FileDropDown.h"
#include "MainFrame.h"
#include <wx/dnd.h>
#include <wx/msgdlg.h>

FileDropDown::FileDropDown(MainFrame* frame): parentFrame(frame)
{

}

bool FileDropDown::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    if (filenames.IsEmpty()) return false;

    // Get the first dropped file path
    wxString filePath = filenames[0];

    wxString ext = filePath.AfterLast('.').Lower();

    if (ext == "png" || ext == "jpg" || ext == "jpeg")
    {
        parentFrame->HandleDroppedFile(filePath);
        return true;
    }
    else
    {
        // Reject the drop with a message box
        wxMessageBox("Invalid file type! Please drop an image file (.png, .jpg, .jpeg).",
            "Unsupported Format", wxOK | wxICON_ERROR);
        return false;
    }
}
