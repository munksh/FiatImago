Name:       harbour-fiatimago
Summary:    Develops RAWfish RAW files and photos
Version:    1.0
Release:    1
License:    MIT
URL:        https://github.com/munksh/FiatImago
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  desktop-file-utils

%description
fiat imago develops the RAW files RAWfish saves, and ordinary JPEG photos:
light, colour, crop, sharpness and vignette, previewed live. The original is
never changed; every export is a new JPEG.

%if 0%{?_chum}
Title: Fiat Imago
Type: desktop-application
DeveloperName: Munkstolen
Categories:
 - Graphics
 - Photography
AIRating: V
AINote: Claude is my typist - I cross review with Mistral, and add the code once it looks good. Architecture, design, on-device testing, releases and maintenance by me; issues and input welcome.
PackageIcon: https://munkstolen.se/SFOS/harbour-fiatimago.png
Screenshots:
 - https://munkstolen.se/SFOS/fiatimago1.png
 - https://munkstolen.se/SFOS/fiatimago2.png
 - https://munkstolen.se/SFOS/fiatimago3.png
 - https://munkstolen.se/SFOS/fiatimago4.png
 - https://munkstolen.se/SFOS/fiatimago5.png
 - https://munkstolen.se/SFOS/fiatimago6.png
Custom:
  Repo: https://github.com/munksh/FiatImago
Links:
  Homepage: https://github.com/munksh/FiatImago
  Bugtracker: https://github.com/munksh/FiatImago/issues
%endif

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 APP_VERSION=%{version}
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install INSTALL_ROOT=%{buildroot}
desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
