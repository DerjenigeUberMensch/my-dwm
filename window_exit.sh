#!/bin/bash

# Get the ID of the currently active (top) window using xprop
active_window_id=$(xprop -root | grep "_NET_ACTIVE_WINDOW(WINDOW)" | awk '{print $5}')

if [[ -n "$active_window_id" ]]; then
     xdotool getwindowfocus windowclose
fi
