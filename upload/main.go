package main

import (
	"fmt"
	"log"
	"net/http"

	"github.com/gin-gonic/gin"
)

func setupRouter() *gin.Engine {
	// Disable Console Color
	// gin.DisableConsoleColor()
	router := gin.Default()
	router.MaxMultipartMemory = 8 << 20 // 8 MiB
	router.POST("/up", func(ctx *gin.Context) {
		file, _ := ctx.FormFile("file")
		save_path := fmt.Sprintf("/opt/config/%s", file.Filename)
		log.Println(save_path)
		ctx.SaveUploadedFile(file, save_path)
		ctx.String(http.StatusOK, "ok")
	})
	return router
}

func main() {
	r := setupRouter()
	// Listen and Server in 0.0.0.0:8080
	r.Run(":8080")
}
