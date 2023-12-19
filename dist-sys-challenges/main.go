package main

import (
	"bufio"
	"fmt"
	"os"

	"github.com/mahmednabil109/dist-sys-challenges/services"
)

func main() {

	node := services.NewNode()
	scanner := bufio.NewScanner(os.Stdin)

	for scanner.Scan() {
		response, err := node.HandleMessage(scanner.Bytes())
		if err != nil {
			fmt.Fprintf(os.Stderr, "error happend will handling: %v", err)
			break
		}
		fmt.Fprintln(os.Stdout, string(response))
	}
}
