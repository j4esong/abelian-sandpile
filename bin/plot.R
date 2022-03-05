
script.dir <- dirname(sys.frame(1)$ofile)
freqDist <- read.table(paste(script.dir, "/freqDist.txt", sep = ""))
simFileName <- paste(script.dir, "/simInfo.txt", sep = "")
connection <- file(simFileName, open = "r")
simInfo <- readLines(connection)

infoStr <- paste("drops = ", simInfo[4],
                 " (", simInfo[6], " random, ", simInfo[5], " centered)", 
                 ", init. ", simInfo[1], 
                 sep = "")

pdf(paste(script.dir, "/sandpileplot.pdf", sep = ""))

plot(freqDist[[1]], freqDist[[2]], 
     main = paste("Abelian Sandpile (", simInfo[2], "x", simInfo[3], ") Avalanche Size Frequency", sep = ""), 
     xlab = "Avalanche Size", 
     ylab = "Frequency", 
     xlim = c(0, max(freqDist[[1]])),
     sub = infoStr,
     font.sub = 3)

plot(freqDist[[1]], log(freqDist[[2]]), 
     main = paste("Abelian Sandpile (", simInfo[2], "x", simInfo[3], ") Avalanche Size Frequency", sep = ""),
     xlab = "Avalanche Size", 
     ylab = "ln(Frequency)",
     xlim = c(0, max(freqDist[[1]])),
     sub = infoStr,
     font.sub = 3)

close(connection)
dev.off()