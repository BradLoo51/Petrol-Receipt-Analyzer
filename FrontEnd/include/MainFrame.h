#include <wx/wx.h>
#include <wx/webrequest.h>
#include <wx/grid.h>
#include <wx/activityindicator.h>

class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString& title);

	void HandleDroppedFile(const wxString& filePath);
private:
	void OnUploadClicked(wxCommandEvent& event);
	void OnDropZoneClicked(wxMouseEvent& event);
	void OnSaveClicked(wxCommandEvent& event);
	wxString EncodeImageToBase64(const wxString& filePath);

	void OnImagePanelPaint(wxPaintEvent& event);
	void OnImageMouseWheel(wxMouseEvent& event);

	void SetupUploadScreen();
	void SetupTableScreen();
	void SetupLoadingScreen();
	void SetupMasterScreen();
	void ResetUploadScreen();
	void BindEventHandlers();

	

	wxPanel* mainPanel;
	wxPanel* uploadScreen;
	wxPanel* tableScreen;
	wxPanel* loadingScreen;
	wxActivityIndicator* loadingSpinner;
	wxStaticText* loadingText;
	wxBoxSizer* masterSizer;
	wxGrid* table;
	wxButton* uploadButton;
	wxPanel* dropZonePanel;
	wxStaticText* dropText;
	wxStaticBitmap* fileIcon;
	wxString selectedFilePath;
	wxButton* saveButton;
	wxPanel* imageCanvasPanel;
	wxImage rawReceiptImage;
	double zoomScale = 1.0;
};
