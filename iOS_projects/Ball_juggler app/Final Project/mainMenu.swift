//
//  mainMenu.swift
//  Final Project
//
//  Created by Samuel Lee on 4/24/15.
//  Copyright (c) 2015 Samuel Lee. All rights reserved.
//

import UIKit
import SpriteKit
import Parse
import Bolts

class mainMenu: SKScene {
   
    
    let startGame: SKLabelNode = SKLabelNode(text: "Start")
    let highScore: SKLabelNode = SKLabelNode(text: "HighScore")
    var highscoreArray: [Int] = [0,0,0]
    
    
    
    override func didMoveToView(view: SKView) {
        
        // parse data
        
        var query = PFQuery(className: "highscore")
        query.findObjectsInBackgroundWithBlock { (objects: [AnyObject]!, error: NSError!) -> Void in
            if error == nil {
                
                for object in objects {
                    println("Activated")
                    var score = object.objectForKey("score") as Int
                    println("\(score)")
                    self.highscoreArray.append(score)
                    
                    
                }
                
            }
        }
        
        self.backgroundColor = UIColor.whiteColor()
        
        startGame.name = "start"
        highScore.name = "highscore"
        var labelArray = [startGame, highScore]
        var midX = CGRectGetMidX(self.frame)
        var midY = CGRectGetMidY(self.frame)
        var offset = CGFloat(10)
        
        for label in 0...labelArray.count-1 {
        var fontsize = CGFloat(32)
        labelArray[label].fontName = "Verdana"
        labelArray[label].fontColor = UIColor.blackColor()
        labelArray[label].fontSize = fontsize
        labelArray[label].position = CGPoint(x: midX, y: (midY + offset) - ((fontsize + offset) * (CGFloat(label))))
        self.addChild(labelArray[label])
    }
        
        
    
        
        
        
    }
    
    
    
    override func touchesBegan(touches: NSSet, withEvent event: UIEvent) {
        
        var touch = touches.anyObject() as? UITouch
        var location = touch?.locationInNode(self)
        if location != nil {
        var node = self.nodeAtPoint(location!)
            if node.name == "start" {
                var scene = GameScene()
                scene.size = self.size
                scene.scaleMode = .Fill
                var transition = SKTransition.crossFadeWithDuration(NSTimeInterval(2.0))
                self.view?.presentScene(scene, transition: transition)
                
            }
            else if node.name == "highscore" {
                var scene = highscoresMenu()
                scene.highscoreArray = self.highscoreArray
                scene.size = self.size
                scene.scaleMode = .Fill
                var transition = SKTransition.crossFadeWithDuration(NSTimeInterval(1.0))
                self.view?.presentScene(scene, transition: transition)
            }
        }
    }
    
    
    
    
    
    
    
    
}
