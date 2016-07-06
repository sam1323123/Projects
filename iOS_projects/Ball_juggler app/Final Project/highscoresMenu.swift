//
//  highscoresMenu.swift
//  Final Project
//
//  Created by Samuel Lee on 4/24/15.
//  Copyright (c) 2015 Samuel Lee. All rights reserved.
//

import UIKit
import SpriteKit
import Parse
import Bolts

class highscoresMenu: SKScene {
    
    
    var highscoreArray: [Int] = [0, 0, 0]
    var nodesArray: [SKLabelNode] = []
    
    override func didMoveToView(view: SKView) {
        
        
        self.backgroundColor = UIColor.whiteColor()
        
        println(self.highscoreArray)
        
  
        
        self.highscoreArray.sort({ (first, second) -> Bool in
            
            return first > second
        })

        
        
        var startX = CGRectGetMidX(self.frame)
        var offset = CGFloat(30)
        var startY = CGRectGetMidY(self.frame) + offset
        var fontsize = CGFloat(20)
        var gap = fontsize + CGFloat(10)
        
       
        for i in 0...2 {  // create highscore labels
            
            var label: SKLabelNode = SKLabelNode(fontNamed: "Verdana")
            label.fontSize = 20
            label.fontColor = UIColor.blackColor()
            label.name = "\(i)"
            label.text = "\(i+1).) "
            label.position = CGPoint(x: startX, y: (startY - (CGFloat(i) * gap)))
            nodesArray.append(label)
            self.addChild(label)
        }
        
        // create back button
        
        var backNode: SKLabelNode = SKLabelNode(fontNamed: "Verdana")
        backNode.fontSize = fontsize
        backNode.fontColor = UIColor.blackColor()
        backNode.color = UIColor.blueColor()
        backNode.text = "Back"
        backNode.name = "Back"
        backNode.position = CGPoint(x: startX, y: offset)
       
        self.addChild(backNode)
        
        self.addScore() // add score values to label nodes
        
        
    }
    
    func addScore() {
        
        for i in 0...2 {
            
            var score = self.highscoreArray[i]
            for label in nodesArray {
                
                if label.name == "\(i)" {  // add scores to label
                 
                    label.text = label.text + "\(score)"
                                    }
            }
            
            
        }
        
    }
    
    
    override func touchesBegan(touches: NSSet, withEvent event: UIEvent) {
        
        var touch = touches.anyObject() as? UITouch
        var location = touch?.locationInNode(self)
        
        if location != nil {
            
            var node = self.nodeAtPoint(location!)
            if node.name == "Back" {
                
                var scene = mainMenu()
                scene.size = self.size
                scene.scaleMode = .Fill
                var transition = SKTransition.crossFadeWithDuration(NSTimeInterval(1.0))
                self.view?.presentScene(scene, transition: transition)
            }
        }
        
    }
    
    
   
}
