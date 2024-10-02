<?php
	plugin_header();

	$nf     =   (strpos($PAGE, '_x32') > 0) ? 32 : 16;
	$m      =   (strpos($PAGE, '_mono') > 0) ? 'm' : (
	            (strpos($PAGE, '_stereo') > 0) ? 's' : (
                (strpos($PAGE, '_lr') > 0) ? 'lr' : (
                (strpos($PAGE, '_ms') > 0) ? 'ms' : '?'
	            )));
	$cc     =   ($m == 'm') ? 'mono' : 'stereo';
?>
<?php require_once("${DOC_BASE}/manuals/common/filters.php"); ?>
<p>
	There are some recommendations that could be given when applying equalization:
</p>
<ul>
	<li><b>Resonance filter</b> with high Quality Factor can be good choice to cut annoying masking resonances from the original sound.</li>
	<li><b>Bell filter</b> with medium Quality Factor can be used when there is necessity to remove short range of frequencies.</li>
	<li><b>Bell filter</b> with low Quality Factor can be used to raise or lower wide range of frequencies.</li>
	<li><b>Shelving filters</b> with low Quality Factor also can be used to lower or raise the large diapasone of frequencies.</li>
	<li><b>Low-pass</b> and <b>High-pass filters</b> can be used with a bit raised Quality Factor to flatten frequency fall. Usage of
	Butterworth-Chebyshev low-pass and high-pass filters with 2x and 3x slope can give good results when cutting unwanterd low and high
	frequencies from voice or guitar sound.
	</li>
	<li><b>Matched Z Transform (MT) filters</b> are probably the best choice when cutting out individual short range of frequencies.</li>
	<li><b>Bilinear Transform (BT) filters</b> are good when cutting-out high frequencies because they have -INF dB amplification at the Nyquist frequency.</li>
    <li><b>Direct Design (DR) filters</b> add alterantive implementations for the various supperted filter types, and may be chosen whenever their frequency response is best suited.</li>
</ul>
<p>
	This plugin performs parametric equalization of <?= $cc ?> channel<?php
	if ($m == 'ms') echo " in Mid-Side mode";
	elseif ($m == 'lr') echo " by applying individual equalization to left and right channels separately";
	?>. Up to <?= $nf ?> filters are available for signal processing simultaneously.
</p>
<p>
	Additional filter inspection feature with the controlled width of the bandpass filter allows to listen the frequency
	band around several filter types to make search and cut off resonances much easier:
</p>
<? if (($m == 'ms') || ($m == 'lr')) { ?>
<p>
	It is also possible to switch filter between <? if ($m == 'ms') echo "Mid and Side"; else "Left and Right"; ?> channels
	by selecting corresponding menu item in the popup menu (
		<? if ($m == 'ms') echo "Switch to Mid / Switch to Side";
		   else "Switch to Left / Switch to Right"; ?>
	) above the filter's dot on the graph.
</p>
<? } ?>

<p>The steepness of the filter is depending on the type of the filter selected, it's slope and Q factor.<p>
<p>There is a simple rule that allows to compute the steepness of IIR (recursive) low-pass or high-pass filter which consists
of so-called poles and zeros:<p>
<ul>
<li>each pole gives -6 dB/octave magnitude attenuation above the pole's frequency;</li>
<li>each zero gives +6 dB/octave magnitude amplification above the zero's frequency.</li>
</ul>
<p>That allows to make the following conclusions:</p>
<ul>
<li>the more poles are used by the filter, the more steep magnitude attenuation will be;</li>
<li>when talking about the magnitude, each zero cancels the pole's attenuation curve and vice verse, depending on their locations in the frequency domain;</li>
<li>the filter can not contain more zeros than the number of poles. Otherwise it will give infinite magnitude at the infinite frequency;</li>
<li>the slope of simple filers is a discrete value with step of -6 dB/octave.</li>
</ul>
<p>There is a table that specifies characteristics of lowpass filters depending on the selected filter mode and slope.</p>
<table>
<tr>
	<th rowspan="2">Mode</th>
	<th colspan="4">Poles</th>
	<th colspan="4">Slope (dB/oct)</th>
</tr>
<tr>
	<th>x1</th>
	<th>x2</th>
	<th>x3</th>
	<th>x4</th>
	<th>x1</th>
	<th>x2</th>
	<th>x3</th>
	<th>x4</th>
</tr>
<tr>
	<td>RLC</td>
	<td>2</td>
	<td>4</td>
	<td>6</td>
	<td>8</td>
	<td>-12</td>
	<td>-24</td>
	<td>-36</td>
	<td>-48</td>
</tr>
<tr>
	<td>BWC</td>
	<td>2</td>
	<td>4</td>
	<td>6</td>
	<td>8</td>
	<td>-12</td>
	<td>-24</td>
	<td>-36</td>
	<td>-48</td>
</tr>
<tr>
	<td>LRX</td>
	<td>4</td>
	<td>8</td>
	<td>12</td>
	<td>16</td>
	<td>-24</td>
	<td>-48</td>
	<td>-72</td>
	<td>-96</td>
</tr>
<tr>
	<td>APO</td>
	<td>2</td>
	<td>2</td>
	<td>2</td>
	<td>2</td>
	<td>-12</td>
	<td>-12</td>
	<td>-12</td>
	<td>-12</td>
</tr>
</table>

<p>Adjusting q can give additional steepness to the curve but making it too high gives extra non-linearity of the magnitude in the passed spectrum.</p>

<ul>
<li>Bell-shaped filters;</li>
<li>Notch filters;</li>
<li>Resonance filters;</li>
<li>Hi-shelving filters;</li>
<li>Lo-shelving filters.</li>
</ul>
<p>
	The frequency control feature allows to detect the note and the note detune the filter is operating with at the top
	of the graph area.
</p>
<p>
	Additionally, mouse double-click on the graph area allows to add a filter with desired frequency and gain
	settings depending on the actual frequency value at the position of the mouse click:
</p>
<ul>
	<li>20 Hz and below: Hi-pass RLC BT filter with Q=0.5;</li>
	<li>20 Hz - 50 Hz: Lo-shelf RLC BT filter with Q=0.5;</li>
	<li>50 Hz - 5 kHz: Bell RLC BT filter with Q=2;</li>
	<li>5 kHz - 15 kHz: Hi-shelf RLC BT filter with Q=0.5;</li>
	<li>15 kHz and above: Lo-pass RLC BT filter with Q=0.5.</li>
</ul>
<p>
	Several filter parameters like type, mode, slope, inspection, solo and mute are also available for change from the
	context menu which can be displayed by the right mouse click over the filter's control dot in the graph area.
</p>
<p><b>Meters:</b></p>
<ul>
	<?php if ($m == 'ms') { ?>
		<li><b>Input</b> - the level meter for left and right channels of the input signal.
		If <b>Listen</b> button is pressed, it shows the level of middle and side channels of the input signal respectively.</li>
		<li><b>Output</b> - the level meter for left and right channels of the output signal.
		If <b>Listen</b> button is pressed, it shows the level of middle and side channels of the output signal respectively.</li>
	<?php } elseif ($m != 'm') { ?>
		<li><b>Input</b> - the level meter for left and right channels of the input signal.</li>
		<li><b>Output</b> - the level meter for left and right channels of the output signal.</li>
	<?php } else { ?>
		<li><b>Input</b> - the level meter of the input mono signal.</li>
		<li><b>Output</b> - the level meter of the output mono signal.</li>
	<?php } ?>
</ul>
<p><b>Controls:</b></p>
<ul>
	<li>
		<b>Bypass</b> - bypass switch, when turned on (led indicator is shining), the plugin bypasses signal.
	</li>
	<li><b>Mode</b> - equalizer working mode, enables the following mode for all filters:</li>
	<ul>
		<li><b>IIR</b> - Infinite Impulse Response filters, nonlinear minimal phase. In most cases does not add noticeable latency to output signal.</li>
		<li><b>FIR</b> - Finite Impulse Response filters with linear phase, finite approximation of equalizer's impulse response. Adds noticeable latency to output signal.</li>
		<li><b>FFT</b> - Fast Fourier Transform approximation of the frequency chart, linear phase. Adds noticeable latency to output signal.</li>
		<li><b>SPM</b> - Spectral Processor Mode of equalizer, equalizer transforms the magnitude of signal spectrum instead of applying impulse response to the signal.</li>
	</ul>
	<?php if ($m == 'ms') { ?>
	<li><b>Mid</b> - button enables the frequency chart and FFT analysis for the middle channel, knob allows to adjust the level of the middle channel.</li>
	<li><b>Side</b> - button enables the frequency chart and FFT analysis for the side channel, knob allows to adjust the level of the side channel.</li>
	<li><b>Listen</b> - allows to listen middle channel and side channel. Passes middle channel to the left output channel, side channel to the right output channel.</li>
	<?php } elseif ($m != 'm') { ?>
	<li><b>Left</b> - enables the <?php if ($m != 's') echo "frequency chart and "; ?>FFT analysis for the left channel.</li>
	<li><b>Right</b> - enables the <?php if ($m != 's') echo "frequency chart and "; ?>FFT analysis for the right channel.</li>
	<?php } ?>
	<li><b>Zoom</b> - zoom fader, allows to adjust zoom on the frequency chart.</li>
	<li><b>Inspect</b> - the button that disables the inspect mode for currently selected filter.</li>
	<li><b>Auto</b> - the button that enables automatic filter inspection for the currently edited filter.</li>
	<li><b>Inspect knob</b> - the knob that controls the bandwidth of the bandpass filter around the inspected filter.</li>
</ul>
<p><b>'Signal' section:</b></p>
<ul>
	<li><b>Input</b> - input signal amplification.</li>
	<li><b>Output</b> - output signal amplification.</li>
	<?php if ($m != 'm') { ?>
	<li><b>Balance</b> - balance between left and right output channels.</li>
	<?php } ?>
	<?php if ($m == 'ms') { ?>
	<li><b>Pitch Mid</b> - the frequency shift for all filters of the middle channel, in semitones.</li>
	<li><b>Pitch Side</b> - the frequency shift for all filters of the side channel, in semitones.</li>
	<?php } elseif ($m == 'lr' ) { ?>
	<li><b>Pitch Left</b> - the frequency shift for all filters of the left channel, in semitones.</li>
	<li><b>Pitch Right</b> - the frequency shift for all filters of the right channel, in semitones.</li>
	<?php } else { ?>
	<li><b>Pitch</b> - the frequency shift for all filters, in semitones.</li>
	<?php }?>
</ul>
<p><b>'Analysis' section:</b></p>
<ul>
	<li><b>FFT</b> - enables FFT analysis before or after processing.</li>
	<li><b>Reactivity</b> - the reactivity (smoothness) of the spectral analysis.</li>
	<li><b>Shift</b> - allows to adjust the overall gain of the analysis.</li>
</ul>
<p><b>'Filters' section:</b></p>
<ul>
	<li><b>Filter</b> - sets up the mode of the selected filter. Currently available filters:</li>
	<ul>
		<li><b>Off</b> - Filter is not working (turned off).</li>
		<li><b>Bell</b> - Bell filter with smooth peak/recess.</li>
		<li><b>Hi-pass</b> - High-pass filter with rejection of low frequencies.</li>
		<li><b>Hi-shelf</b> - Shelving filter with adjustment of high frequency range.</li>
		<li><b>Lo-pass</b> - Low-pass filter with rejection of high frequencies.</li>
		<li><b>Lo-shelf</b> - Shelving filter with adjustment of low frequencies.</li>
		<li><b>Notch</b> - Notch filter with full rejection of selected frequency.</li>
		<li><b>Resonance</b> - Resonance filter wih sharp peak/recess.</li>
		<li><b>Allpass</b> - All-pass filter which only affects the phase of the audio signal at the specified frequency.</li>
	</ul>
	<li><b>Mode</b> - sets up the class of the filter:</li>
	<ul>
		<li><b>RLC</b> - Very smooth filters based on similar cascades of RLC contours.</li>
		<li><b>BWC</b> - Butterworth-Chebyshev-type-1 based filters. Does not affect <b>Resonance</b> and <b>Notch</b> filters.</li>
		<li><b>LRX</b> - Linkwitz-Riley based filters. Does not affect <b>Resonance</b> and <b>Notch</b> filters.</li>
        <li><b>APO</b> - Digital biquad filters derived from canonic analog biquad prototypes digitalized through
        				Bilinear transform. These are <a href="https://shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html">textbook filters</a>
        				which are implemented as in the <a href="https://equalizerapo.com/">EqualizerAPO</a> software.</li>
		<li><b>BT</b> - Bilinear Z-transform is used for pole/zero mapping.</li>
		<li><b>MT</b> - Matched Z-transform is used for pole/zero mapping.</li>
        <li><b>DR</b> - Direct design is used to serve the digital filter coefficients directly in the digital domain, without performing transforms.</li>
	</ul>
	<li><b>Slope</b> - the slope of the filter characteristics.</li>
	<li><b>I</b> - the inspection button, allows to inspect selected filter.</li>
	<li><b>S</b> - the soloing button, allows to solo selected filters.</li>
	<li><b>M</b> - the mute button, allows to mute selected filters.</li>
	<li><b>Freq</b> - the cutoff/resonance frequency of the filter.</li>
	<li><b>Gain</b> - the gain of the filter, disabled for lo-pass/hi-pass/notch filters.</li>
	<li><b>Q</b> - the quality factor of the filter.</li>
	<li><b>Hue</b> - the color of the frequency chart of the filter on the graph.</li>
</ul>
