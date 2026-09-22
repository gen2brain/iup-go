//go:build js && wasm && media

package iup

// MediaOpen must be called after Open, so that the media elements can be used.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_audio.html
func MediaOpen() {
	ccall("IupMediaOpen", "number", nil, nil)
}

// Audio creates an audio player.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_audio.html
func Audio() Ihandle {
	return ccallHandle("IupAudio", nil, nil)
}

// Camera creates a canvas that shows the live picture of a camera.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_camera.html
func Camera() Ihandle {
	return ccallHandle("IupCamera", nil, nil)
}

// Microphone creates an audio capture source.
//
// https://gen2brain.github.io/iup-go/ctrl/iup_microphone.html
func Microphone() Ihandle {
	return ccallHandle("IupMicrophone", nil, nil)
}
