#include "MainFrame.h"
#include "FileDropDown.h"
#include <fstream>
#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/wfstream.h>
#include <wx/artprov.h>
#include <wx/base64.h>
#include <wx/webrequest.h>
#include <wx/sstream.h>
#include <nlohmann/json.hpp>
#include <wx/icon.h>
#include <wx/activityindicator.h>

using json = nlohmann::json;



MainFrame::MainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title)
{
	#ifdef __WXMSW__
		SetIcon(wxIcon(wxT("IDI_ICON1")));
	#else
		SetIcon(wxIcon(wxT("assets/app_icon.png"), wxBITMAP_TYPE_PNG));
	#endif

	mainPanel = new wxPanel(this);
	masterSizer = new wxBoxSizer(wxVERTICAL);
	SetupUploadScreen();
	SetupTableScreen();
	SetupLoadingScreen();
	SetupMasterScreen();
	BindEventHandlers();
}


void MainFrame::HandleDroppedFile(const wxString& filePath)
{
	selectedFilePath = filePath;

	wxString fileName = filePath.AfterLast('\\').AfterLast('/');

	wxClientDC dc(dropText);
	wxString shortName = wxControl::Ellipsize(fileName, dc, wxELLIPSIZE_MIDDLE, 250);

	dropText->SetLabel("Selected: " + shortName);

	wxBitmap successIcon = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(48, 48));
	fileIcon->SetBitmap(successIcon);

	dropText->GetParent()->Layout();
}


void MainFrame::OnImagePanelPaint(wxPaintEvent& event)
{
	wxPaintDC dc(imageCanvasPanel);

	if (!rawReceiptImage.IsOk())
	{
		dc.SetTextForeground(*wxWHITE);
		dc.DrawText("No Image Loaded", 20, 20);
		return;
	}

	// 1. Calculate the scaled dimensions based on current zoom wheel multiplier
	int newWidth = static_cast<int>(rawReceiptImage.GetWidth() * zoomScale);
	int newHeight = static_cast<int>(rawReceiptImage.GetHeight() * zoomScale);

	// 2. Perform a fast, clean scaling interpolation optimization transformation
	wxImage scaledImg = rawReceiptImage.Scale(newWidth, newHeight, wxIMAGE_QUALITY_NORMAL);
	wxBitmap bmp(scaledImg);

	// 3. Center the image inside the panel canvas viewport boundaries
	wxSize panelSize = imageCanvasPanel->GetSize();
	int xPos = (panelSize.GetWidth() - newWidth) / 2;
	int yPos = (panelSize.GetHeight() - newHeight) / 2;

	// 4. Draw the image to the screen pixels
	dc.DrawBitmap(bmp, xPos, yPos, true);
}

void MainFrame::OnImageMouseWheel(wxMouseEvent& event)
{
	if (event.GetWheelRotation() > 0)
	{
		zoomScale += 0.1;
	}
	else
	{
		zoomScale -= 0.1;
		if (zoomScale < 0.1) zoomScale = 0.1;
	}

	imageCanvasPanel->Refresh();
}

void MainFrame::SetupUploadScreen()
{
	uploadScreen = new wxPanel(mainPanel, wxID_ANY);

	dropZonePanel = new wxPanel(uploadScreen, wxID_ANY, wxDefaultPosition,
								wxSize(600, 300), wxBORDER_SIMPLE);
	dropZonePanel->SetBackgroundColour(*wxLIGHT_GREY);
	uploadButton = new wxButton(uploadScreen, wxID_ANY, "Upload", wxDefaultPosition, wxSize(100, 40));

	wxBitmap defaultIcon = wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_OTHER, wxSize(48, 48));
	fileIcon = new wxStaticBitmap(dropZonePanel, wxID_ANY, defaultIcon);

	dropText = new wxStaticText(dropZonePanel, wxID_ANY, "Drop the Receipt Here",
								wxDefaultPosition, wxDefaultSize);

	wxFont font = dropText->GetFont();
	font.SetPointSize(14);
	dropText->SetFont(font);
	dropText->SetCursor(wxCursor(wxCURSOR_HAND));

	wxFont buttonFont = uploadButton->GetFont();
	buttonFont.SetPointSize(12);
	uploadButton->SetFont(buttonFont);

	wxBoxSizer* boxInnerSizer = new wxBoxSizer(wxVERTICAL);
	boxInnerSizer->AddStretchSpacer(1);
	boxInnerSizer->Add(fileIcon, 0, wxALIGN_CENTER | wxBOTTOM, 10);
	boxInnerSizer->Add(dropText, 0, wxALIGN_CENTER);
	boxInnerSizer->AddStretchSpacer(1);

	dropZonePanel->SetSizer(boxInnerSizer);
	dropZonePanel->SetDropTarget(new FileDropDown(this));
	dropZonePanel->SetCursor(wxCursor(wxCURSOR_HAND));

	wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
	outerSizer->AddStretchSpacer(1);
	outerSizer->Add(dropZonePanel, wxSizerFlags().Border(wxALL, 25).Centre());
	outerSizer->Add(uploadButton, wxSizerFlags().Border(wxBOTTOM, 50).Centre());
	outerSizer->AddStretchSpacer(1);

	uploadScreen->SetSizer(outerSizer);
	
}


void MainFrame::OnUploadClicked(wxCommandEvent& event)
{
	if (selectedFilePath.IsEmpty())
	{
		wxMessageBox("Please select or drop a valid receipt image before uploading!",
			"No File Selected",
			wxOK | wxICON_WARNING | wxCENTRE, this);
		return; // Exits the function immediately, keeping the user on the upload screen
	}

	wxString base64Image = EncodeImageToBase64(selectedFilePath);

	if (base64Image.IsEmpty()) {
		wxMessageBox("Failed to read image file.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	json imagePayload;
	imagePayload["image"] = { base64Image.ToStdString() }; // One array element holding the base64 string
	wxString jsonPayLoad(imagePayload.dump());

	wxString url = "http://localhost:8000/extract";

	wxWebRequest request = wxWebSession::GetDefault().CreateRequest(this, url);
	if (!request.IsOk()) return;

	request.SetData(jsonPayLoad, "application/json");

	Bind(wxEVT_WEBREQUEST_STATE, [this](wxWebRequestEvent& evt) {
		switch (evt.GetState())
		{
			// Request completed
			case wxWebRequest::State_Completed:
			{

				try
				{
					std::string rawResponse = evt.GetResponse().AsString().ToStdString();

					json receiptData = json::parse(rawResponse);
	
					int newRow = table->GetNumberRows();
					table->AppendRows(1);

					table->SetCellValue(newRow, 0, wxString::FromUTF8(receiptData.value("brand", "")));
					table->SetCellValue(newRow, 1, wxString::FromUTF8(receiptData.value("fuel_type", "")));
					table->SetCellValue(newRow, 2, wxString::FromUTF8(wxString::Format("%.2f", receiptData.value("quantity", 0.0))));
					table->SetCellValue(newRow, 3, wxString::FromUTF8(wxString::Format("%.2f", receiptData.value("gross_amount", 0.0))));
					table->SetCellValue(newRow, 4, wxString::FromUTF8(wxString::Format("%.2f", receiptData.value("final_amount", 0.0))));
					table->SetCellValue(newRow, 5, wxString::FromUTF8(receiptData.value("date", "")));

					table->AutoSizeRows();
				}
				catch (const json::parse_error& e)
				{
					// Stop the loader and return to upload if parsing breaks
					loadingSpinner->Stop();
					loadingScreen->Hide();
					uploadScreen->Show();
					mainPanel->Layout();

					wxMessageBox("JSON Data Parsing Error: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR);
					break;
				}

				rawReceiptImage.LoadFile(selectedFilePath, wxBITMAP_TYPE_ANY);

				loadingSpinner->Stop();
				loadingScreen->Hide();
				tableScreen->Show();

				// Layout UI Transition
				// 1. Grab and lock down the exact current window dimensions
				wxSize originalSize = this->GetSize();

				// 2. Temporarily lift minimum structural restrictions
				this->SetMinSize(wxSize(-1, -1));

				// 3. Force layout recalculation and set the new minimum constraint limits
				mainPanel->Layout();
				masterSizer->SetSizeHints(this);

				// 4. Instantly force the window frame to snap back to its exact pre-click size
				this->SetSize(originalSize);

				if (rawReceiptImage.IsOk() && imageCanvasPanel)
				{
					wxSize canvasSize = imageCanvasPanel->GetSize();
					double scaleX = static_cast<double>(canvasSize.GetWidth()) / rawReceiptImage.GetWidth();
					double scaleY = static_cast<double>(canvasSize.GetHeight()) / rawReceiptImage.GetHeight();

					// Choose the smaller ratio to fit the entire receipt fully on screen cleanly
					zoomScale = (scaleX < scaleY) ? scaleX : scaleY;
					imageCanvasPanel->Refresh();
				}
				break;
			}
			case wxWebRequest::State_Failed:
			{
				loadingSpinner->Stop();
				loadingScreen->Hide();
				uploadScreen->Show();
				mainPanel->Layout();

				wxLogError("Could not send request: %s", evt.GetErrorDescription());
				break;
			}
			default:
				break;

				
		}
	});

	request.SetTimeouts(300000, 300000);

	uploadScreen->Hide();
	loadingScreen->Show();
	loadingSpinner->Start();
	mainPanel->Layout();

	request.Start();

}


void MainFrame::OnDropZoneClicked(wxMouseEvent& event)
{
	wxClientDC dc(dropText);
	dc.SetFont(GetFont());
	
	// Configure the native file browser dialog
	wxFileDialog openFileDialog(this, "Select Receipt Image", "", "",
		"Image Files (*.png;*.jpg;*.jpeg;)|*.png;*.jpg;*.jpeg;",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	selectedFilePath = openFileDialog.GetPath();

	wxString fileName = openFileDialog.GetFilename();
	wxString shortName = wxControl::Ellipsize(fileName, dc,
		wxELLIPSIZE_MIDDLE, 250);
	dropText->SetLabel("Selected: " + shortName);

	wxBitmap successIcon = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(48, 48));
	fileIcon->SetBitmap(successIcon);

	dropText->GetParent()->Layout();
}

void MainFrame::OnSaveClicked(wxCommandEvent& event)
{
	std::string exportFilePath = "../Petrol_Receipts.csv";
	bool fileExists = std::ifstream(exportFilePath).good();

	std::ofstream outFile(exportFilePath, std::ios::app);
	if (!outFile.is_open()) {
		wxMessageBox("Could not open file for saving. Please check if the Excel file is open elsewhere!",
			"Save Error", wxOK | wxICON_ERROR);
		return;
	}

	// Write column headers ONLY if the file is being brand new created for the first time
	if (!fileExists) {
		outFile << "Brand,Fuel Type,Quantity,Gross Amount,Final Amount,Date\n";
	}

	int totalRows = table->GetNumberRows();
	for (int row = 0; row < totalRows; ++row)
	{
		outFile << "\"" << table->GetCellValue(row, 0).ToStdString() << "\","  // Brand
			<< "\"" << table->GetCellValue(row, 1).ToStdString() << "\","  // Fuel Type
			<< table->GetCellValue(row, 2).ToStdString() << ","            // Quantity
			<< table->GetCellValue(row, 3).ToStdString() << ","            // Gross Amount
			<< table->GetCellValue(row, 4).ToStdString() << ","            // Final Amount
			<< table->GetCellValue(row, 5).ToStdString() << "\n";          // Date
	}
	outFile.close();

	tableScreen->Hide();
	uploadScreen->Show();

	ResetUploadScreen();

	mainPanel->Layout();
}


wxString MainFrame::EncodeImageToBase64(const wxString& filePath)
{
	wxFileInputStream fileStream(filePath);
	if (!fileStream.IsOk()) return "";

	size_t fileSize = fileStream.GetLength();
	std::vector<char> buffer(fileSize);
	fileStream.Read(buffer.data(), fileSize);

	return wxBase64Encode(buffer.data(), fileSize);
}


void MainFrame::SetupTableScreen()
{
	tableScreen = new wxPanel(mainPanel, wxID_ANY);
	tableScreen->Hide();

	table = new wxGrid(tableScreen, wxID_ANY);
	table->SetDefaultColSize(100, true);
	table->CreateGrid(0, 6);

	table->SetColLabelValue(0, "Brand");
	table->SetColLabelValue(1, "Fuel Type");
	table->SetColLabelValue(2, "Quantity");
	table->SetColLabelValue(3, "Gross Amount");
	table->SetColLabelValue(4, "Final Amount");
	table->SetColLabelValue(5, "Date");

	wxFont cellFont = table->GetDefaultCellFont();
	cellFont.SetPointSize(12);
	table->SetDefaultCellFont(cellFont);

	saveButton = new wxButton(tableScreen, wxID_ANY, "Save to Excel (CSV)", wxDefaultPosition, wxSize(150, 40));
	wxFont saveFont = saveButton->GetFont();
	saveFont.SetPointSize(11);
	saveFont.SetWeight(wxFONTWEIGHT_BOLD);
	saveButton->SetFont(saveFont);

	wxBoxSizer* leftLayoutSizer = new wxBoxSizer(wxVERTICAL);
	leftLayoutSizer->Add(table, 1, wxEXPAND | wxALL, 5);
	leftLayoutSizer->Add(saveButton, 0, wxALIGN_RIGHT | wxALL, 10);

	imageCanvasPanel = new wxPanel(tableScreen, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
	imageCanvasPanel->SetBackgroundColour(*wxBLACK);
	

	wxBoxSizer* tableSizer = new wxBoxSizer(wxHORIZONTAL);
	tableSizer->Add(leftLayoutSizer, 3, wxEXPAND | wxALL, 10);
	tableSizer->Add(imageCanvasPanel, 2, wxEXPAND | wxALL, 10);

	tableScreen->SetSizer(tableSizer);
}

void MainFrame::SetupLoadingScreen()
{
	loadingScreen = new wxPanel(mainPanel, wxID_ANY);
	loadingScreen->Hide();

	loadingSpinner = new wxActivityIndicator(loadingScreen, wxID_ANY);

	loadingText = new wxStaticText(loadingScreen, wxID_ANY, "Analyzing Receipt with Gemma4...",
						wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

	wxFont font = loadingText->GetFont();
	font.SetPointSize(13);
	font.SetWeight(wxFONTWEIGHT_BOLD);
	loadingText->SetFont(font);

	wxBoxSizer* loadingSizer = new wxBoxSizer(wxVERTICAL);
	loadingSizer->AddStretchSpacer(1);
	loadingSizer->Add(loadingSpinner, 0, wxALIGN_CENTER | wxBOTTOM, 15);
	loadingSizer->Add(loadingText, 0, wxALIGN_CENTER);
	loadingSizer->AddStretchSpacer(1);

	loadingScreen->SetSizer(loadingSizer);
}


void MainFrame::SetupMasterScreen()
{
	masterSizer->Add(uploadScreen, 1, wxEXPAND);
	masterSizer->Add(loadingScreen, 1, wxEXPAND);
	masterSizer->Add(tableScreen, 1, wxEXPAND);

	mainPanel->SetSizer(masterSizer);
	masterSizer->Layout();
	masterSizer->SetSizeHints(this);
	
}

void MainFrame::ResetUploadScreen()
{
	selectedFilePath.Clear();

	if (table->GetNumberRows() > 0) {
		table->DeleteRows(0, table->GetNumberRows());
	}

	dropText->SetLabel("Drop the Receipt Here");

	wxBitmap defaultIcon = wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_OTHER, wxSize(48, 48));
	fileIcon->SetBitmap(defaultIcon);

	rawReceiptImage = wxImage();
	zoomScale = 1.0;
}


void MainFrame::BindEventHandlers()
{
	uploadButton->Bind(wxEVT_BUTTON, &MainFrame::OnUploadClicked, this);
	dropZonePanel->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnDropZoneClicked, this);
	dropText->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnDropZoneClicked, this);
	fileIcon->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnDropZoneClicked, this);
	saveButton->Bind(wxEVT_BUTTON, &MainFrame::OnSaveClicked, this);
	imageCanvasPanel->Bind(wxEVT_PAINT, &MainFrame::OnImagePanelPaint, this);
	imageCanvasPanel->Bind(wxEVT_MOUSEWHEEL, &MainFrame::OnImageMouseWheel, this);
}
