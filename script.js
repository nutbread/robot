(function () {
	"use strict";

	var on_ready = (function () {

		// Vars
		var callbacks = [],
			check_interval = null,
			check_interval_time = 250;

		// Check if ready and run callbacks
		var callback_check = function () {
			if (
				(document.readyState === "interactive" || document.readyState === "complete") &&
				callbacks !== null
			) {
				// Run callbacks
				var cbs = callbacks,
					cb_count = cbs.length,
					i;

				// Clear callbacks, events, checking interval
				callbacks = null;

				window.removeEventListener("load", callback_check, false);
				window.removeEventListener("DOMContentLoaded", callback_check, false);
				document.removeEventListener("readystatechange", callback_check, false);

				if (check_interval !== null) {
					clearInterval(check_interval);
					check_interval = null;
				}

				// Run callbacks
				for (i = 0; i < cb_count; ++i) {
					cbs[i].call(null);
				}

				// Okay
				return true;
			}

			// Not executed
			return false;
		};

		// Listen
		window.addEventListener("load", callback_check, false);
		window.addEventListener("DOMContentLoaded", callback_check, false);
		document.addEventListener("readystatechange", callback_check, false);

		// Callback adding function
		return function (cb) {
			if (callbacks === null) {
				// Ready to execute
				cb.call(null);
			}
			else {
				// Delay
				callbacks.push(cb);

				// Set a check interval
				if (check_interval === null && callback_check() !== true) {
					check_interval = setInterval(callback_check, check_interval_time);
				}
			}
		};

	})();

	var on_audio_click = function (event) {
		if (!event.which || event.which !== 1) return;

		var url = this.getAttribute("href"),
			status = this.nextSibling,
			audio, on_play, on_stop;

		if (status === null || !status.classList || !status.classList.contains("audio_status")) {
			status = document.createElement("span");
			if (!status.classList) return;

			audio = document.createElement("audio");
			audio.style.display = "none";

			status.className = "audio_status";

			this.parentNode.insertBefore(audio, this.nextSibling);
			this.parentNode.insertBefore(status, this.nextSibling);

			on_play = function () { status.textContent = " (playing)"; };
			on_stop = function () { status.textContent = " (stopped)"; };

			audio.addEventListener("play", on_play, false);
			audio.addEventListener("pause", on_stop, false);
			audio.addEventListener("ended", on_stop, false);
			audio.addEventListener("error", function () {
				if (audio.parentNode !== null) {
					audio.parentNode.removeChild(audio);
					status.textContent = " (error)";
				}
			}, false);

			audio.autoplay = true;
			audio.src = url;
			on_play();
		}
		else {
			audio = status.nextSibling;
			if (audio.tagName !== "AUDIO") return;

			try {
				audio.currentTime = 0;
				audio.play();
			}
			catch (e) {}
		}

		event.preventDefault();
		event.stopPropagation();
		return false;
	};

	on_ready(function () {
		var links = document.querySelectorAll("a.audio[href]"),
			i;

		for (i = 0; i < links.length; ++i) {
			links[i].addEventListener("click", on_audio_click, false);
		}
	});

})();



