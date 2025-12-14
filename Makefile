VERSION="R3sw1989"
ARCHIVE_URL=https://archive.org/download/TheAdventuresOfCaptainComic/AdventuresOfCaptainComicEpisode1The-PlanetOfDeath${VERSION}michaelA.Denioaction.zip

get_original:
	echo "Downloading ${ARCHIVE_URL} via curl..."
	curl -L -o zip/${VERSION}.zip ${ARCHIVE_URL}
	
	echo "Extracting archive to orig/..."
	unzip -o "zip/${VERSION}.zip" -d "orig/"