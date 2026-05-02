$ErrorActionPreference = 'Stop'
$OutDir = "c:\labs engine finish tn\app\third-party\webview2"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$Url = "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2420.47"
$ZipFile = "$OutDir\webview2.zip"
Write-Host "Downloading $Url..."
Invoke-WebRequest -Uri $Url -OutFile $ZipFile
Write-Host "Extracting..."
Expand-Archive -Path $ZipFile -DestinationPath $OutDir -Force
Remove-Item -Path $ZipFile
Write-Host "Done."
