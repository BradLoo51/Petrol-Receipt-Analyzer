from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import ollama
import base64
from datetime import date, datetime
from pydantic import BaseModel, Field, field_validator
import uvicorn

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

class ImagePayload(BaseModel):
    image: list[str]

class Receipt(BaseModel):
    brand: str
    fuel_type: str
    quantity: float
    gross_amount: float
    final_amount: float
    date: str = Field(description="The raw date string exactly as it appears on the receipt, matching the format DD/MM/YY. Do NOT separate or omit the forward slashes and only provide the date number and nothing else.")

@field_validator("date")
@classmethod
def convert_to_date(cls, v: str) -> date:
    # Strip out accidental spaces if the LLM leaves any
    clean_str = v.replace(" ", "")
    # Adjust format string to "%m/%d/%y" if your receipts use Month/Day/Year
    return datetime.strptime(clean_str, "%d/%m/%y").date()

@app.post("/extract")
async def extract(payload: ImagePayload):
    try:
        image_bytes = base64.b64decode(payload.image[0])
        
        response = ollama.chat(
            model = "gemma4:26b",
            messages = [
                {
                    "role": "user", 
                    "content": "Extract the necessary information from the receipt.",
                    "images": [image_bytes]
                },
            ],
            format=Receipt.model_json_schema(),
        )

        print(Receipt.model_validate_json(response.message.content))

        return Receipt.model_validate_json(response.message.content)

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8000)