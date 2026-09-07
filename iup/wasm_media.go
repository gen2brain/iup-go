//go:build js && wasm && media

package iup

// MediaOpen must be called after Open, so that the media elements can be used.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_audio.md
func MediaOpen() {
	ccall("IupMediaOpen", "number", nil, nil)
}

// Audio creates an audio player.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_audio.md
func Audio() Ihandle {
	return ccallHandle("IupAudio", nil, nil)
}

// Camera creates a canvas that shows the live picture of a camera.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_camera.md
func Camera() Ihandle {
	return ccallHandle("IupCamera", nil, nil)
}

// Microphone creates an audio capture source.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_microphone.md
func Microphone() Ihandle {
	return ccallHandle("IupMicrophone", nil, nil)
}
