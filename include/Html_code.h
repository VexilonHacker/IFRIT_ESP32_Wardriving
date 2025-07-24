#ifndef HTML_CODE_H
#define HTML_CODE_H

#include <Config.h>

String BuildHtmlCode(String data, int currentBrightness) {
    String html_code = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>)rawliteral" + String(WEBPAGE_TITLE) + R"rawliteral(</title>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <style>
        body {
          font-family: monospace;
          background: #111;
          color: #0f0;
          padding: 20px;
          font-size: 22px;
        }
        h1 {
          font-size: 28px;
          margin-bottom: 20px;
        }
        pre {
          font-size: 16px;
          white-space: pre-wrap;
          word-wrap: break-word;
        }
        .slider-container {
          margin-top: 40px;
        }
        input[type=range] {
          width: 100%;
          height: 8px;
          border-radius: 3px;
          background: linear-gradient(to right, #0f0 100%, #444 0%);
          -webkit-appearance: none;
          outline: none;
        }
        input[type=range]::-webkit-slider-runnable-track {
          background: transparent;
          height: 6px;
        }
        input[type=range]::-webkit-slider-thumb {
          -webkit-appearance: none;
          background: #0f0;
          border: none;
          width: 18px;
          height: 18px;
          border-radius: 50%;
          cursor: pointer;
          margin-top: -6px;
        }
        input[type=range]::-moz-range-thumb {
          background: #0f0;
          border: none;
          width: 18px;
          height: 18px;
          border-radius: 50%;
          cursor: pointer;
        }
        input[type=range]::-moz-range-track {
          background: transparent;
          height: 6px;
        }
        .reset-container {
          margin-top: 30px;
        }
        #resetButton {
          font-size: 18px;
          padding: 10px 20px;
          background: #222;
          color: #0f0;
          border: 1px solid #0f0;
          cursor: pointer;
        }
        #resetStatus {
          font-size: 14px;
          color: #0a0;
          margin-top: 8px;
        }
      </style>
    </head>
    <body>
      <h1>IFRIT Status</h1>
      <pre id='statusData'>)rawliteral" + data + R"rawliteral(</pre>

      <div class='slider-container'>
        <label for='brightnessSlider'>Brightness: <span id='brightnessValue'>)rawliteral" + String(currentBrightness) + R"rawliteral(</span></label><br>
        <input type='range' min=)rawliteral" + String(MIN_BRIGHTNESS) + R"rawliteral( max=)rawliteral" + String(MAX_BRIGHTNESS) + R"rawliteral( value=)rawliteral" + String(currentBrightness) + R"rawliteral( id='brightnessSlider'>
        <p style='font-size: 14px; color: #00cc00; margin-top: 8px;'>
          Decreasing brightness helps increase battery lifetime.
        </p>
      </div>

      <div class='reset-container'>
        <button id='resetButton'>Reset IFRIT</button>
        <p id='resetStatus'></p>
      </div>

      <script>
        let slider = document.getElementById('brightnessSlider');
        let output = document.getElementById('brightnessValue');
        let statusPre = document.getElementById('statusData');

        function updateSliderColor(val) {
          const percent = (val / 255) * 100;
          slider.style.background = `linear-gradient(to right, #0f0 ${percent}%, #444 ${percent}%)`;
        }

        slider.oninput = function() {
          output.innerText = this.value;
          updateSliderColor(this.value);
          fetch('/setBrightness?value=' + this.value).catch(e => console.log(e));
        };

        async function fetchStatusData() {
          try {
            const response = await fetch('/statusData');
            if (response.ok) {
              const text = await response.text();
              statusPre.textContent = text;
            }
          } catch (error) {
            console.error('Error fetching status data:', error);
          }
        }

        document.getElementById('resetButton').addEventListener('click', async () => {
          const resetStatus = document.getElementById('resetStatus');
          try {
            const res = await fetch('/reset');
            if (res.ok) {
              resetStatus.textContent = "Reset command sent successfully.";
            } else {
              resetStatus.textContent = "Failed to send reset command.";
            }
          } catch (err) {
            resetStatus.textContent = "Error contacting device.";
            console.error(err);
          }
        });

        updateSliderColor(slider.value);
        fetchStatusData();
        setInterval(fetchStatusData, )rawliteral" + String(WEBPAGE_DELAY) + R"rawliteral();
      </script>
    </body>
    </html>
    )rawliteral";
    return html_code;
}

#endif // HTML_CODE_H
