---
title: Mainboard
layout: home
parent: Build
nav_order: 1
---

<h1>Mainboard Assembly Guide</h1>
<h4>This page guides through the process of wiring up an skr pico to the bambu labs a1.</h4>

----

<h2>1. Removing toolhead and side hub wiring</h2>
<h4>Start by removing the live camera and all connectors that travel through the hub on the side.</h4>

![HubRemoval](/Images/Hardware/hub_removal_1.jpg){: width="250" }
![HubRemoval](/Images/Hardware/camera_and_sleeve_1.jpg){: width="250" }

<h5>Source: Bambu Labs Wiki</h5>


<h4>The current state of the printer should be as follows:
    <br>All wires going to toolhead and x motor hub should be removed
    <br>Nylon sleeve removed
</h4>

----

<h2>2. Wiring the Motors</h2>
<h4>Remove the bigger bottom base of the 3d printer, there are 10 M2 hex screws around the edge
    <br>You have a choice in what to do.
    <br>- Get some 16 gauge wires or above and crimp them with fish hole crimps, length above 40cm at minimum
    <br>- Desolder the wires from the mainboard and solder some wires to make it longer, allowing skipping getting crimps
    <br>
    <br>Route the Y (not coloured) and Z (yellow) cables so they can be accessed from the ouside. You may need to remove the smaller baseplate from the printer.
    <br>Route the psu cables to the outer side.
    <br>Seal up the bottom base of the printer, making sure they are all accessable from outside, giving enough room for bending.
</h4>

----

<h3>3. Toolhead wiring</h3>
<h4>Some stuff routes from the toolhead to the mb, it will be plugged in later.
    <br>Plug them into the pico ports for the motors and the 24V in. [need image].
    <br>Confirm that the colours match up:
    <br>- X = Yellow
    <br>- Y = Not Coloured
    <br>- Z = Green
</h4>
<image src="Images/Pinout/mainboard_plugging.png">

----
