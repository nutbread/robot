' Usage:
'   C:\Windows\SysWOW64\cscript //Nologo voice.vbs [arguments]



' Constants
Const SVSFDefault = 0



' Argument checking
Dim argv
Set argv = WScript.Arguments

If argv.Length = 1 And argv(0) = "list-voices" Then
	' List voices
	WScript.StdOut.WriteLine(enum_voices())
	WScript.Quit(0)
End If

If argv.Length < 5 Then
	WScript.StdErr.WriteLine("Usage")
	WScript.StdErr.WriteLine("  cscript " & Wscript.ScriptName & " list-voices")
	WScript.StdErr.WriteLine("  or")
	WScript.StdErr.WriteLine("  cscript " & Wscript.ScriptName & " <output-wav> <voice-name> <volume> <voice-rate> <channels> <sample-rate>")
	WScript.Quit(-1)
End If

output_wav = argv(0)
voice_name = argv(1)
voice_volume = str_to_int(argv(2), 100)
voice_rate = str_to_int(argv(3), 0)
voice_channels = str_to_int(argv(4), 2)
voice_samples_rate = str_to_int(argv(5), 44100)



' Speaking
Call speak(WScript.StdIn, output_wav, voice_name, voice_volume, voice_rate, voice_channels, voice_samples_rate)



' Done
WScript.Quit(0)



' Argument conversion
Function str_to_int(str, def)
	On Error Resume Next
	str_to_int = def
	str_to_int = CInt(str)
End Function

' Voice enumeration
Function enum_voices()
	Set speaker = CreateObject("SAPI.SpVoice")
	Set voices = speaker.GetVoices
	Set speaker = Nothing

	result = ""
	first = True
	For Each voice_str in voices
		If first Then
			first = False
		Else
			result = result & vbLf
		End If
		result = result & voice_str.GetAttribute("Name")
	Next

	enum_voices = result
End Function

' Speaking
Function speak(stream, filename, voice, volume, rate, channels, samples)
	' Speaker and output file
    Set speaker = CreateObject("SAPI.SpVoice")
	Set output = CreateObject("SAPI.SpFileStream")
	output.format.Type = get_wave_type(channels, samples)
	Call output.Open(filename, 3)
	Set speaker.AudioOutputStream = output

	' Setup speaker
	Call set_voice(speaker, voice)
	speaker.Rate = rate
	speaker.Volume = volume

	' Read
	While Not stream.AtEndOfStream
		line = stream.ReadLine
		Call speak_text(speaker, line)
	Wend

	' Done
	Set speaker = Nothing
End Function

Function speak_text(speaker, text)
    On Error Resume Next
	Call speaker.Speak(text, SVSFDefault)
End Function

Function get_wave_type(channels, samples)
	' https://msdn.microsoft.com/en-us/library/ms720595.aspx
	get_wave_type = 34

	If samples =  8000 Then
		get_wave_type = 6
	ElseIf samples = 16000 Then
		get_wave_type = 18
	ElseIf samples = 22050 Then
		get_wave_type = 22
	ElseIf samples = 44100 Then
		get_wave_type = 34
	ElseIf samples = 48000 Then
		get_wave_type = 38
	End If

	If channels = 2 Then
		get_wave_type = get_wave_type + 1
	End If
End Function

' Speaker setup
Function set_voice(speaker, voice)
	If Len(voice) > 0 Then
		Set voice_list = speaker.GetVoices("Name=" & voice)
		If voice_list.Count = 1 Then
			Set speaker.Voice = voice_list.Item(0)
		End If
	End If
End Function


