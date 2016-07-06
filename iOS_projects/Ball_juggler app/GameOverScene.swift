//
//  GameOverScene.swift
//  Final Project
//
//  Created by Samuel Lee on 4/16/15.
//  Copyright (c) 2015 Samuel Lee. All rights reserved.
//

import UIKit
import SpriteKit

class GameOverScene: SKScene {
    
    
    override func didMoveToView(view: SKView) {
       
        /* Setup your scene here */
        
        println(self.frame)
         let myLabel = SKLabelNode(fontNamed:"Chalkduster")
        myLabel.text = "Game Over";
        myLabel.fontSize = 30;
        myLabel.position = CGPoint(x:CGRectGetMidX(self.frame), y:CGRectGetMidY(self.frame));
        //myLabel.position = CGPoint(x: 20, y: 20)
        
        self.addChild(myLabel)
        
        // screen setup
        
        var startX = CGRectGetMidX(self.frame)
        var startY = CGFloat(30)
        
        var backNode: SKLabelNode = SKLabelNode(fontNamed: "Verdana")
        backNode.fontSize = CGFloat(30)
        backNode.fontColor = UIColor.whiteColor()
        backNode.text = "Main Menu"
        backNode.name = "Menu"
        backNode.position = CGPoint(x: startX, y: startY)
        
        self.addChild(backNode)
        
    }
    
    override func touchesBegan(touches: NSSet, withEvent event: UIEvent) {
        
        var touch = touches.anyObject() as? UITouch
        
        if touch != nil {
            var location = touch?.locationInNode(self)
            var node = self.nodeAtPoint(location!)
            if node.name == "Menu" {
               
                var scene = mainMenu()
                scene.size = self.size
                self.view?.presentScene(scene)
                
            }
            else {
        var scene = GameScene()
        scene.size = self.size
        self.view?.presentScene(scene)
            }
    }
    }
}
