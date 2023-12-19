package services

import (
	"bytes"
	"errors"
)

type NodeHandleFunc func([]byte) ([]byte, error)

type Node struct {
	NodeId        string
	NextNodeIds   []string
	nextMessageId int32
	nodeTypeTabel map[string]NodeHandleFunc
}

func NewNode() (node *Node) {
	node = &Node{
		nextMessageId: 1,
	}
	node.nodeTypeTabel = map[string]NodeHandleFunc{
		InitMessageType:     node.HandleInit,
		EchoMessageType:     node.HandleEcho,
		GenerateMessageType: node.HandleGenerate,
	}
	return
}

func (n *Node) GetNextMessageId() (id int32) {
	id = n.nextMessageId
	n.nextMessageId++
	return
}

func (n *Node) HandleMessage(message []byte) ([]byte, error) {
	idx := bytes.Index(message, []byte("type"))
	if idx == -1 {
		return nil, errors.New("can't interpert message with no type")
	}
	eidx := bytes.Index(message[idx:], []byte(","))
	if eidx == -1 {
		return nil, errors.New("can't interpert message")
	}

	messageType := string(message[idx+7 : idx+eidx-1])
	nodeFunc, ok := n.nodeTypeTabel[messageType]
	if !ok {
		return nil, errors.New("can't handle message with unkown type: " + messageType)
	}
	response, err := nodeFunc(message)
	return response, err
}
