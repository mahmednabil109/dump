package main

import (
	"bufio"
	"encoding/json"
	"os"
	_ "fmt"
)

var msg_id int32 = 1

type EchoMessage struct {
	Src  string `json:"src"`
	Dest string `json:"dest"`
	Body struct {
		Type  string `json:"type"`
		MsgId int32  `json:"msg_id"`
		Echo  string `json:"echo"`
	} `json:"body"`
}

type EchoOkMessage struct {
	Src  string `json:"src"`
	Dest string `json:"dest"`
	Body struct {
		Type      string `json:"type"`
		MsgId     int32  `json:"msg_id"`
		InReplyTo int32  `json:"in_reply_to"`
		Echo      string `json:"echo"`
	} `json:"body"`
}

type InitMessage struct {
	Src  string `json:"src"`
	Dest string `json:"dest"`
	Body struct {
		Type    string   `json:"type"`
		MsgId   int32    `json:"msg_id"`
		NodeId  string   `json:"node_id"`
		NodeIds []string `json:"node_ids"`
	} `json:"body"`
}

type InitOkMessage struct {
	Src  string `json:"src"`
	Dest string `json:"dest"`
	Body struct {
		Type      string `json:"type"`
		InReplyTo int32  `json:"in_reply_to"`
	} `json:"body"`
}

func main() {
	defer func() {
		if r := recover(); r != nil {
			// propbably EOF
			// fmt.Println(r)
		}
	}()

	reader := bufio.NewReader(os.Stdin)
	decoder := json.NewDecoder(reader)

	writer := bufio.NewWriter(os.Stdout)
	encoder := json.NewEncoder(writer)

	var msg map[string]any
	var err error
	for err = decoder.Decode(&msg); err == nil; err = decoder.Decode(&msg) {
		if err != nil {
			panic(err)
		}
		bytes, err := json.Marshal(msg)
		if err != nil {
			panic(err)
		}
		// just writting the echo Node
		body, _ := msg["body"].(map[string]any)
		msg_type, _ := body["type"].(string)
		switch msg_type {
		case "echo":
			echo_msg := EchoMessage{}	
			if err := json.Unmarshal(bytes, &echo_msg); err != nil {
				panic(err)
			}

			msg_ok := EchoOkMessage{}
			msg_ok.Src, msg_ok.Dest = echo_msg.Dest, echo_msg.Src
			msg_ok.Body.Type = "echo_ok"
			msg_ok.Body.MsgId = msg_id
			msg_ok.Body.InReplyTo = echo_msg.Body.MsgId
			msg_ok.Body.Echo = echo_msg.Body.Echo

			if err = encoder.Encode(msg_ok); err != nil {
				panic(err)
			}
		case "init":
			init_msg := InitMessage{}
			if err := json.Unmarshal(bytes, &init_msg); err != nil {
				panic(err)
			}

			init_ok := InitOkMessage{}
			init_ok.Src, init_ok.Dest = init_msg.Dest, init_msg.Src
			init_ok.Body.Type = "init_ok"
			init_ok.Body.InReplyTo = init_msg.Body.MsgId

			if err = encoder.Encode(init_ok); err != nil {
				panic(err)
			}
		}
		msg_id += 1
		writer.Flush()
	}
	writer.Flush()
	panic(err)
}
