# Audio

## Recommended direction

Use SDL_mixer 3 on top of the existing SDL3 platform layer when the audio
milestone begins. It is an official, zlib-licensed companion library and provides
mixing, streaming, format conversion, effects, and support for formats including
WAV, MP3, and Ogg Vorbis. Pin it like other source dependencies and do not rely on
whatever codecs happen to be installed on the player's operating system.

Keep gameplay outside the library API:

```text
gameplay -> AudioSystem semantic commands -> SDL_mixer implementation -> SDL3
```

Examples of semantic commands are `play_collision()`, `set_engine_state()`, and
`play_music()`. Suggested initial buses are master, music, engines, effects, and
UI. Short repeated effects should be decoded and retained in memory; music and
long ambience should be streamed.

Do not add SDL_mixer merely to make silence in the current track-relative physics
milestone. Add and verify it with the first audible checkpoint.

## Source and shipping formats

Do not use MP3 as the only master copy. Keep original recordings and delivered
music in lossless WAV or FLAC, preferably 48 kHz; retain the highest sensible bit
depth supplied by the creator. Generate game-ready files from those masters.

Recommended first shipping policy:

- short sound effects: WAV, preloaded and mixed in memory;
- music and long ambience: Ogg Vorbis, streamed;
- voice, if it becomes substantial: evaluate Ogg Vorbis or Opus at that milestone.

MP3 is technically supported, but it provides no particular advantage for this
game. The historic Fraunhofer/Technicolor core MP3 patent-licensing program ended
in 2017 after the last patents in that program expired. That does not grant rights
to a recording or composition, nor does it replace checking the license of the
exact encoder, decoder, library, sample pack, or commissioned work being used.
Keep purchase receipts, licenses, contributor agreements, and source masters.
This project guidance is not legal advice.

Never transcode a lossy MP3 into another lossy shipping format if a lossless
master can be obtained; repeated lossy encoding reduces quality.

## Protecting shipped audio

A game installed on a player's machine cannot make its playable audio impossible
to extract. The executable must be able to obtain the decoded samples, so a
determined user can obtain them too. Encryption with a key embedded in the game
only raises the effort slightly.

A later build archive can bundle assets for tidy deployment, validation, and a
small barrier against casual browsing, but it must not be described as DRM or a
security boundary. Do not build a proprietary archive system during the current
prototype. The meaningful protections are original work, clear copyright and
contract records, shipping compressed derivatives rather than production
masters, and ordinary platform enforcement when redistribution occurs.

## Dynamic vehicle audio

Vehicle sound should be driven by simulation state, not by render frame rate.
Useful normalized inputs will include propulsion, speed, lateral slip, surface
contact, damage, and boost state. Engine character can initially use a small
number of looped layers whose volume and pitch blend smoothly; boost can add a
separate transient and loop rather than merely making everything louder.

## References

- [SDL3 audio overview](https://wiki.libsdl.org/SDL3/CategoryAudio)
- [SDL_mixer 3 overview](https://wiki.libsdl.org/SDL3_mixer/FrontPage)
- [SDL_mixer 3 migration and track model](https://wiki.libsdl.org/SDL3_mixer/README-migration)
- [SDL_mixer loading and streaming](https://wiki.libsdl.org/SDL3_mixer/MIX_LoadAudio_IO)
- [Fraunhofer MP3 licensing-program history](https://www.audioblog.iis.fraunhofer.com/mp3-software-patents-licenses)
- [Xiph Vorbis specification and implementation license](https://xiph.org/vorbis/doc/Vorbis_I_spec.pdf)
