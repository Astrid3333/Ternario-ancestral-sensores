Name:           tritos-compress
Version:        5.0
Release:        1%{?dist}
Summary:        Compresor Ternario Ancestral
License:        MIT
Source0:        tritos_compress

%description
Compresor lossless usando ternario balanceado y CRT.

%install
mkdir -p %{buildroot}/usr/bin
cp %{SOURCE0} %{buildroot}/usr/bin/tritos_compress
chmod 755 %{buildroot}/usr/bin/tritos_compress

%files
/usr/bin/tritos_compress

%changelog
* Sat Sep 13 2026 Ternario Ancestral <ternario@tritos.dev> - 5.0-1
- Compresor ternario ancestral v5 con CRT
