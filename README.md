This is a port of Quake for devices like the RS-97, Arcade Mini, PAP K3S... that rely on SDL 1.2.
(SDL 2.0 is too slow on those platforms)

It used to be based around sdlquake but that port had numerous crashes and issues so i rebased it around TyrQuake,
which was the only suitable base that was not Darkplaces.

TyrQuake only supported SDL2 so i had to reuse some of the sdlquake's code in there but it was mostly compatible with it.

It also runs pretty well on the said platforms, even on the PAP K3S with a screen resolution of 800x480.
