#!/usr/bin/env python3
#### worked one with camera 

import tkinter as tk
from tkinter import Label, Button
from PIL import Image, ImageTk
import time
import os
import threading
import concurrent.futures

# Import Picamera2
from picamera2 import Picamera2
from picamera2.controls import Controls # For camera controls if needed

# Import Gemini API libraries
import google.generativeai as genai
from io import BytesIO
import numpy as np # Picamera2 returns numpy arrays
import cv2 # Import OpenCV here so it's accessible throughout the class

class CameraApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Camera GUI with Gemini Vision (Picamera2)")

        # Define the new color palette based on the image
        self.dark_blue = "#2C4F7F"
        self.medium_blue = "#3B6DAA"
        self.orange_yellow = "#F2A83B"
        self.light_text_color = "white"
        self.neutral_button_blue = "#8DA8C6"

        self.root.geometry("800x700") # Increased height for result label
        self.root.config(bg=self.dark_blue)

        # --- Picamera2 Initialization ---
        self.picam2 = None # Initialize to None

        try:
            self.picam2 = Picamera2()
            print("DEBUG: Picamera2 object created.")
        except Exception as e:
            print(f"CRITICAL ERROR: Failed to create Picamera2 object: {e}")
            raise Exception(f"Failed to create Picamera2 object. Is camera connected/available? {e}")

        # ***** ADDED: Robust Camera Reset Attempt *****
        try:
            if self.picam2.started:
                self.picam2.stop()
            self.picam2.close()
            # Re-initialize the Picamera2 object after closing
            self.picam2 = Picamera2()
        except Exception as e:
            print(f"WARNING: Error during pre-start camera reset: {e}")
            # If reset fails, we'll try to proceed, but it might lead to the original error.
            # No re-raise here, as the subsequent steps will likely catch a persistent error.
        # **********************************************

        # Configure the camera to include BOTH 'main' and 'lores' streams
        camera_config = None # Initialize to None
        try:
            camera_config = self.picam2.create_video_configuration(
                main={"size": (1280, 720), "format": "RGB888"}, # High-res for capture
                lores={"size": (640, 480), "format": "RGB888"}  # Low-res for live feed
            )
        except Exception as e:
            # Attempt to close if it was opened
            if self.picam2 and self.picam2.started:
                self.picam2.stop()
            if self.picam2:
                self.picam2.close()
            raise Exception(f"Failed to create camera configuration: {e}") # Re-raise

        try:
            if camera_config: # Only configure if config was successfully created
                self.picam2.configure(camera_config)
            else:
                raise ValueError("Camera configuration was not created due to a previous error.")
        except Exception as e:
            if self.picam2 and self.picam2.started:
                self.picam2.stop()
            if self.picam2:
                self.picam2.close() # Ensure camera is closed on config error
            raise Exception(f"Failed to apply camera configuration: {e}") # Re-raise

        try:
            self.picam2.start()
        except Exception as e:
            # If camera fails to start, ensure it's closed before raising.
            if self.picam2 and self.picam2.started:
                self.picam2.stop()
            if self.picam2:
                self.picam2.close() # Ensure it's closed even on startup failure
            raise Exception(f"Failed to initialize Camera: {e}") # Re-raise to stop program
        # --- End Picamera2 Initialization ---

        self.captured_image = None
        self.gemini_client = None
        self.is_capturing = True # Controls if live feed is active
        self.analysis_executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        self.analysis_future = None
        self.analysis_requested = False # Flag to indicate if an analysis has been confirmed by user

        # !!! IMPORTANT: Replace with your actual Gemini API key !!!
        self.api_key = "YOUR_GEMINI_API_KEY_HERE" # <--- REPLACE THIS LINE
        if self.api_key == "YOUR_GEMINI_API_KEY_HERE":
            print("You can get one from https://makers.generativeai.google/key")
        try:
            genai.configure(api_key=self.api_key)
            self.gemini_client = genai
        except Exception as e:
            print(f"Error configuring Gemini API: {e}")
            self.gemini_client = None

        # This line is crucial for clean shutdown. It binds the window close event
        # to your custom on_closing method.
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

        self.canvas = Label(root, bg=self.dark_blue)
        self.canvas.pack(pady=10)

        self.button_frame = tk.Frame(root, bg=self.dark_blue)
        self.button_frame.pack(pady=10)

        self.capture_button = Button(self.button_frame, text="Capture", command=self.capture_photo,
                                     font=("Arial", 14), bg=self.medium_blue, fg=self.light_text_color,
                                     relief="flat", bd=0, padx=20, pady=10,
                                     activebackground="#2D5A8A", activeforeground=self.light_text_color)
        self.capture_button.pack(side=tk.LEFT, padx=10)

        self.confirm_button = Button(self.button_frame, text="Confirm", command=self.start_analysis,
                                     font=("Arial", 14), bg=self.orange_yellow, fg=self.light_text_color,
                                     relief="flat", bd=0, padx=20, pady=10,
                                     activebackground="#D18F2E", activeforeground=self.light_text_color)
        self.confirm_button.pack_forget()

        self.retake_button = Button(self.button_frame, text="Retake", command=self.retake_photo,
                                     font=("Arial", 14), bg=self.neutral_button_blue, fg=self.light_text_color,
                                     relief="flat", bd=0, padx=20, pady=10,
                                     activebackground="#7D99B5", activeforeground=self.light_text_color)
        self.retake_button.pack_forget()

        self.result_label = Label(root, text="Analysis results will appear here.",
                                     font=("Arial", 12), bg=self.dark_blue, fg=self.light_text_color,
                                     wraplength=750, justify=tk.LEFT)
        self.result_label.pack(pady=10, padx=20)


        self.root.bind('<q>', self.quit_program)

        # Only start feed if camera was successfully initialized
        if self.picam2:
            self.update_feed() # Start the live camera feed
        else:
            self.result_label.config(text="ERROR: Camera not initialized. Check console for details.")


    def on_closing(self):
        if self.analysis_executor:
            self.analysis_executor.shutdown(wait=True)

        self.release_camera()
        self.root.destroy()

    def update_feed(self):
        # Only proceed if picam2 is initialized and currently capturing
        if self.is_capturing and self.picam2 and self.picam2.started:
            try:
                frame = self.picam2.capture_array("lores")

                if frame is not None:
                    # Ensure frame has enough dimensions before converting and resizing
                    if frame.ndim >= 2:
                        rgb_frame = frame
                        self.frame = rgb_frame # Store for potential later use if needed
                        img = Image.fromarray(rgb_frame)
                        imgtk = ImageTk.PhotoImage(image=img)

                        self.canvas.imgtk = imgtk
                        self.canvas.configure(image=imgtk)
            except Exception as e:
                self.result_label.config(text=f"Live feed error: {e}")
        self.root.after(10, self.update_feed)

    def capture_photo(self):
        if not self.picam2 or not self.picam2.started:
            self.result_label.config(text="ERROR: Camera not active. Cannot capture photo.")
            return

        self.is_capturing = False # Stop live feed
        self.result_label.config(text="Capturing image...")
        try:
            captured_array = self.picam2.capture_array("main")
            self.captured_image = captured_array.copy()

            # Ensure captured_image is a valid array before proceeding
            if self.captured_image is not None and self.captured_image.ndim >= 2:
                h, w = self.captured_image.shape[:2]
                target_width = 800
                target_height = int(target_width * h / w)
                self.captured_image = cv2.resize(self.captured_image, (target_width, target_height), interpolation=cv2.INTER_AREA)

                self.analysis_requested = False
                self.result_label.config(text="Image captured. Press 'Confirm' to analyze or 'Retake'.")

                img = Image.fromarray(self.captured_image)
                imgtk = ImageTk.PhotoImage(image=img)
                self.canvas.imgtk = imgtk
                self.canvas.configure(image=imgtk)

                self.capture_button.pack_forget()
                self.retake_button.pack(side=tk.LEFT, padx=10)
                self.confirm_button.pack(side=tk.LEFT, padx=10)
                self.confirm_button.config(state="normal", text="Confirm")

            else:
                self.result_label.config(text="ERROR: Failed to capture valid image.")
                self.retake_photo() # Go back to live feed if capture failed
        except Exception as e:
            self.result_label.config(text=f"ERROR during photo capture: {e}")
            self.retake_photo() # Go back to live feed if capture failed


    def start_analysis(self):
        if self.analysis_future and self.analysis_future.running():
            self.result_label.config(text="Analysis already in progress. Please wait.")
            return

        if self.captured_image is None or self.captured_image.ndim < 2:
            self.result_label.config(text="ERROR: No valid image to analyze. Please capture an image first.")
            return

        self.analysis_requested = True
        self.confirm_button.config(state="disabled", text="Analyzing...")
        self.retake_button.config(state="disabled")
        self.result_label.config(text="Analyzing image with Gemini Vision AI...")

        self.analysis_future = self.analysis_executor.submit(self._run_analysis_task)
        self.analysis_future.add_done_callback(self._analysis_done_callback)

    def _run_analysis_task(self):
        if not self.gemini_client:
            return {"status": "error", "message": "Gemini API not configured. Please add your API key."}

        # Check analysis_requested again inside the thread just in case
        if not self.analysis_requested:
            return {"status": "cancelled", "message": "Analysis cancelled before API call."}

        # Ensure captured_image is RGB (it should be from Picamera2's RGB888 format)
        # Convert to BGR for OpenCV imencode if it expects BGR
        # Picamera2 RGB888 is already RGB. cv2.imencode is often fine with RGB,
        # but explicit conversion can't hurt if issues arise.
        # For simplicity, if Picamera2 gave RGB, and imencode works with it, no conversion needed here.
        # Let's assume captured_image is already RGB from Picamera2's main stream config.
        # However, for robustness, if imencode complains, convert to BGR first.
        # image_bgr = cv2.cvtColor(self.captured_image, cv2.COLOR_RGB2BGR) # Uncomment if imencode fails with RGB

        # Check if the image array is valid before encoding
        if self.captured_image is None or self.captured_image.size == 0 or self.captured_image.ndim < 2:
            return {"status": "error", "message": "Captured image is invalid or empty for analysis."}


        # For consistency and robustness, let's explicitly convert to BGR for imencode,
        # as imencode often expects BGR by convention with some image types.
        image_to_encode = cv2.cvtColor(self.captured_image, cv2.COLOR_RGB2BGR)

        is_success, buffer = cv2.imencode(".jpg", image_to_encode, [int(cv2.IMWRITE_JPEG_QUALITY), 90])
        if not is_success:
            return {"status": "error", "message": "Error encoding image for Gemini API."}

        image_bytes_data = buffer.tobytes()

        prompt = (
            "You are an AI assistant specialized in medical document analysis. "
            "Carefully examine the provided image. "
            "If the image contains a clear prescription, list **ONLY the names of the medications**. "
            "Present them as a comma-separated list. "
            "If no medications are identifiable or if the image is not a prescription, "
            "state 'No medications found' or describe the general content of the image concisely "
            "if it's clearly not a medical document. "
            "Examples:\n"
            "Prescription: 'Amoxicillin, Ibuprofen, Lisinopril'\n"
            "Not a prescription: 'A document containing various handwritten notes'\n"
            "No medications: 'No medications found'\n"
        )

        try:
            model = self.gemini_client.GenerativeModel('gemini-2.0-flash')

            response = model.generate_content(
                contents=[
                    {
                        "mime_type": "image/jpeg",
                        "data": image_bytes_data
                    },
                    {"text": prompt}
                ]
            )

            if not self.analysis_requested:
                return {"status": "cancelled", "message": "Analysis cancelled after API call."}

            return {"status": "success", "text": response.text}

        except Exception as e:
            return {"status": "error", "message": f"Gemini API call failed: {e}"}

    def _analysis_done_callback(self, future):
        try:
            result = future.result()
            if result["status"] == "success":
                raw_text = result["text"].strip()

                processed_text = raw_text
                final_output = ""

                if "No medications found" in raw_text or "not a medical document" in raw_text.lower() or "describe the main objects" in raw_text.lower():
                    final_output = raw_text
                else:
                    cleaned_text = raw_text
                    common_prefixes = [
                        "Here are the medications: ", "Medications found: ", "The medications are: ",
                        "medications:", "Medications:"
                    ]
                    for prefix in common_prefixes:
                        if cleaned_text.lower().startswith(prefix.lower()):
                            cleaned_text = cleaned_text[len(prefix):].strip()
                            break

                    medications = [m.strip().title() for m in cleaned_text.split(',') if m.strip()]

                    if medications:
                        final_output = "Identified Medications:\n" + "\n".join(medications)
                    else:
                        final_output = "Analysis complete, but no specific medications were clearly identified. Raw response: " + raw_text


                self.result_label.config(text=final_output)

            elif result["status"] == "cancelled":
                self.result_label.config(text=f"Analysis was cancelled: {result['message']}")
            else: # error
                self.result_label.config(text=f"ERROR during analysis: {result['message']}")
        except concurrent.futures.CancelledError:
            self.result_label.config(text="Analysis task was cancelled.")
        except Exception as e:
            self.result_label.config(text=f"An unexpected error occurred: {e}")
        finally:
            if self.analysis_requested: # Only reset buttons if analysis was actually requested (not just cancelled prematurely)
                self.reset_buttons_after_analysis()

    def retake_photo(self):

        self.analysis_requested = False
        if self.analysis_future and self.analysis_future.running():
            self.analysis_future.cancel()
        self.analysis_future = None

        self.captured_image = None
        self.result_label.config(text="Live camera feed.")

        self.confirm_button.pack_forget()
        self.retake_button.pack_forget()
        self.capture_button.pack(side=tk.LEFT, padx=10)
        self.confirm_button.config(state="normal", text="Confirm")

        self.is_capturing = True

    def reset_buttons_after_analysis(self):
        self.confirm_button.config(state="normal", text="Confirm")
        self.retake_button.config(state="normal")

    def release_camera(self):
        """Release the camera resources."""
        if self.picam2: # Only try to stop/close if picam2 object exists
            if self.picam2.started:
                self.picam2.stop()
            self.picam2.close()
            self.picam2 = None # Set to None after closing

    def quit_program(self, event=None):
        self.root.quit()

if __name__ == "__main__":
    app = None # Initialize app to None for proper cleanup in case of __init__ error
    try:
        root = tk.Tk()
        app = CameraApp(root)
        root.mainloop()
    except Exception as e:
        # Only attempt to release camera if the app object was successfully created
        if app and hasattr(app, 'release_camera'):
            app.release_camera()