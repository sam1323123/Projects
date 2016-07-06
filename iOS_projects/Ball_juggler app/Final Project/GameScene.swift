//
//  GameScene.swift
//  Final Project
//
//  Created by Samuel Lee on 3/29/15.
//  Copyright (c) 2015 Samuel Lee. All rights reserved.
//

import SpriteKit
import Parse
import Bolts

class GameScene: SKScene, SKPhysicsContactDelegate {
    
    let backGroundNode: SKSpriteNode = SKSpriteNode(imageNamed: "sky")
    let ball: SKSpriteNode = SKSpriteNode(imageNamed: "ball")
    let floor: SKSpriteNode = SKSpriteNode(imageNamed: "Grass")
    let scoreNode: SKLabelNode = SKLabelNode(fontNamed: "Verdana")
    let instructionNode: SKLabelNode = SKLabelNode(fontNamed: "Verdana")
    var firstTouch: Bool = true
    var score: Int = 0
    var maxHeight: CGFloat = 0// get height of screen
    var midX: CGFloat = 0 // get centre x set later
    var startTouchPt: CGPoint? = nil
    var startTouch: UITouch? = nil
    var endTouchPt: CGPoint? = nil
    var endTouch: UITouch? = nil
    var startTime: NSTimeInterval = 0
    var endTime: NSTimeInterval = 0
    var canHit: Bool = false
    let scalingFactor: CGFloat = 30 // to scale vector quantity by set amount'
    var dxScale: Double = 0
    var dyScale: Double = 0
    let ballCategory: UInt32 = 1 << 0
    let floorCategory: UInt32 = 1 << 1
    let wallCategory: UInt32 = 1 << 2
    let blockCategory: UInt32 = 1 << 3
    var ballKicked = false  // to see whether the ball has touched the ground before
    var blockCount: Int = 0
    var newBlock: SKSpriteNode? = nil
    var blockInSameSpace = false
    //var dx: Float = 0
    //var dy: Float = 0
    //var dTime: Float = 0

    
    
    override func didMoveToView(view: SKView) {
        /* Setup your scene here */
       /* let myLabel = SKLabelNode(fontNamed:"Chalkduster")
        myLabel.text = "Hello, World!";
        myLabel.fontSize = 65;
        myLabel.position = CGPoint(x:CGRectGetMidX(self.frame), y:CGRectGetMidY(self.frame));
        
        self.addChild(myLabel) */
       
        // screen setup
        
        println(self.frame)
        println("scene\(self.scene?.frame)")
        
        self.backgroundColor = UIColor.whiteColor()
      //  self.backGroundNode.size = CGSize(width: self.size.width, height: self.size.height)
       // self.backGroundNode.position = CGPoint(x: CGRectGetMidX(self.frame), y: CGRectGetMidY(self.frame))
        self.physicsWorld.gravity = CGVectorMake(CGFloat(0), CGFloat(-3))
        self.maxHeight = CGRectGetMaxY(self.frame)
        println("maxHeight\(self.maxHeight)")
        self.midX = CGRectGetMidX(self.frame)
        var heightShift: CGFloat = 50
        self.physicsWorld.contactDelegate = self
        
       
        
        // floor setup
        var yShift: CGFloat = -200
        var originalSize = self.floor.size
        var xScale: CGFloat = originalSize.width / CGRectGetMaxX(self.frame)
        var yScale: CGFloat = originalSize.height / CGRectGetMaxY(self.frame)
        var chosenScale: CGFloat = min(xScale, yScale)
        self.floor.setScale(chosenScale)
        self.floor.position = CGPoint(x: self.floor.size.width/2 , y: yShift)  // location of floor position is centre of pic
        self.floor.physicsBody = SKPhysicsBody(rectangleOfSize: self.floor.size)
        self.floor.physicsBody?.dynamic = false
        self.floor.name = "floor"
        self.floor.physicsBody?.categoryBitMask = self.floorCategory
        self.floor.physicsBody?.contactTestBitMask = self.ballCategory
        
        
        // ball setup
        
        self.ball.setScale(CGFloat(0.2))
        self.ball.position = CGPoint(x: CGRectGetMidX(self.frame), y: self.floor.position.y + self.ball.size.width/2 + self.floor.size.height/2)   // refers to centre of object
        self.ball.physicsBody = SKPhysicsBody(circleOfRadius: self.ball.size.width/2)
        self.ball.physicsBody?.dynamic = true
        self.ball.physicsBody?.restitution = 0.5
        self.ball.physicsBody?.allowsRotation = false
        self.ball.physicsBody?.friction = 0.0
        self.ball.physicsBody?.mass = CGFloat(1)
        self.ball.name = "ball"
        self.ball.physicsBody?.categoryBitMask = self.ballCategory
        // self.ball.physicsBody?.collisionBitMask = self.floorCategory | self.wallCategory
        self.ball.physicsBody?.contactTestBitMask = self.floorCategory
        
        // wall setup
        
        var wallFrame: CGRect = self.frame
        var width = CGRectGetWidth(wallFrame)
        var height = CGRectGetHeight(wallFrame)
        var appliedFrame = CGRect(origin: CGPointZero, size: CGSize(width: width, height: height))
        //self.wall.physicsBody = SKPhysicsBody(edgeLoopFromRect: wallFrame)
        //self.wall.physicsBody?.dynamic = false
        self.physicsBody = SKPhysicsBody(edgeLoopFromRect: appliedFrame)
        self.physicsBody?.categoryBitMask = self.wallCategory
        
        // score Label Setup
        self.scoreNode.text = "Score: \(self.score)"
        self.scoreNode.fontSize = 20
        self.scoreNode.fontColor = UIColor.blackColor()
        self.scoreNode.position = CGPoint(x: CGRectGetMidX(self.frame), y: CGRectGetMaxY(self.frame) - CGFloat(30))
        
        
        // Start Screen
        
        self.paused = true
        self.instructionNode.name = "Instruction"
        self.instructionNode.fontSize = 30
        self.instructionNode.fontColor = UIColor.grayColor()
        self.instructionNode.alpha = 0.2
        self.instructionNode.text = "Touch to Begin"
        var instNodeOffset = CGFloat(10)
        self.instructionNode.position = CGPoint(x: CGRectGetMidX(self.frame), y: CGRectGetMidY(self.frame) - instNodeOffset)
        
        

        
      // Add child here
       // self.addChild(self.backGroundNode)
        self.addChild(self.ball)
        self.addChild(self.floor)
        self.addChild(self.scoreNode)
        self.addChild(self.instructionNode)
        
       /* var action = SKAction.runBlock({ () -> Void in
            var block = 1
            if block != 1 {
                
                //self.addChild(block!)
            }
        })
        
        var node = SKNode()
        node.name = "1234"
        node.runAction(SKAction.repeatActionForever(action))
        self.addChild(node)
        */
        // problem with the run block function
        
    }
    
    func didBeginContact(contact: SKPhysicsContact) {
        
        var bodyA = contact.bodyA.categoryBitMask
        var bodyB = contact.bodyB.categoryBitMask
        
        if (bodyA == self.ballCategory && bodyB == self.floorCategory) || (bodyA == self.floorCategory && bodyB == self.ballCategory) {
            
            if self.ballKicked {
            println("Game Over")
            self.gameOver()
            self.ballKicked = false
            }
        }
        
        else if ((bodyA == self.ballCategory && bodyB == self.blockCategory) || (bodyA == self.blockCategory && bodyB == self.ballCategory)) {
            
            if bodyA == self.blockCategory { contact.bodyA.node?.removeFromParent() }
            
            else { contact.bodyB.node?.removeFromParent() }
            
            self.score += 2
            self.scoreNode.text = "Score: \(self.score)"
            self.blockCount -= 1
        }
        
        
    }
    
    func gameOver() {
        
        var newScore: PFObject = PFObject(className: "highscore")
        newScore["score"] = self.score
        newScore.saveInBackground()
        let scene = GameOverScene()
        scene.size = self.size
        scene.scaleMode = .Fill
        var transition = SKTransition.crossFadeWithDuration(NSTimeInterval(1.0))
        self.view?.presentScene(scene, transition: transition)
    }
    
    override func touchesBegan(touches: NSSet, withEvent event: UIEvent) {
        /* Called when a touch begins */
        
       /* for touch: AnyObject in touches {
            let location = touch.locationInNode(self)
            
            let sprite = SKSpriteNode(imageNamed:"Spaceship")
            
            sprite.xScale = 0.5
            sprite.yScale = 0.5
            sprite.position = CGPoint(x: CGRectGetMidX(self.frame), y: CGRectGetMidY(self.frame))
            
            let action = SKAction.rotateByAngle(CGFloat(M_PI), duration:1)
            
            sprite.runAction(SKAction.repeatActionForever(action))
            
            self.addChild(sprite)
        } */
        
        //self.ball.physicsBody?.applyImpulse(CGVector(dx: -1000, dy: 0))
        
        if self.firstTouch {
            
            self.instructionNode.removeFromParent()
            self.spawnBlock()
            self.firstTouch = false
            self.paused = false
        }
        var touch = touches.anyObject() as? UITouch
        self.startTouch = touch
        self.startTouchPt = touch?.locationInNode(self)
        self.startTime = self.startTouch!.timestamp
        
        
    }
    
    
    func createBlock() -> SKSpriteNode {  // create brekable blocks
        
        var block = SKSpriteNode(color: UIColor.redColor() , size: CGSize(width: 40, height: 20))
        block.physicsBody = SKPhysicsBody(rectangleOfSize: block.size)
        block.physicsBody?.dynamic = false
        block.physicsBody?.affectedByGravity = false
        block.physicsBody?.allowsRotation = false
        block.name = "block"
        block.physicsBody?.mass = 1
        block.physicsBody?.categoryBitMask = self.blockCategory
        block.physicsBody?.contactTestBitMask = self.ballCategory
        //block.physicsBody?.collisionBitMask = 0
        
        return block
        
    }
    
    func testIfValidBlock(block: SKSpriteNode) -> Bool {
        
        var blockPosition = block.position
        var blockWidth = block.size.width
        var blockHeight = block.size.height
        var blockStartX = blockPosition.x - (blockWidth/2)
        var blockStartY = blockPosition.y - (blockHeight/2)
        
        for c in self.children {
            
            var child = c as SKNode
            if child.name == "block" {
                var testNode = child as SKSpriteNode
                var nodePosition = testNode.position
                var nodeWidth = testNode.size.width
                var nodeHeight = testNode.size.height
                var startX = nodePosition.x - (nodeWidth/2)
                var startY = nodePosition.y - (nodeHeight/2)  // start point will be at corner of node
                
                // collision test
                if ((startX <= blockStartX && blockStartX <= (startX + nodeWidth)) && (startY <= blockStartY && blockStartY <= (startY + nodeWidth))) { //  bottom left edge
                    
                    return false
                }
                else if ((startX <= (blockStartX + blockWidth) && (blockStartX + blockWidth) <= (startX + nodeWidth)) && (startY <= blockStartY && blockStartY <= (startY + nodeWidth))) {
                    // right bottom edge
                    
                    return false
                }
                
                else if  ((startX <= (blockStartX + blockWidth) && (blockStartX + blockWidth) <= (startX + nodeWidth)) && (startY <= (blockStartY + blockHeight) && (blockStartY + blockHeight) <= (startY + nodeWidth))) {
                    
                    // top right edge
                    return false
                }
                
                else if ((startX <= (blockStartX) && (blockStartX) <= (startX + nodeWidth)) && (startY <= (blockStartY + blockHeight) && (blockStartY + blockHeight) <= (startY + nodeWidth))) {
                    
                    return false
                }
            }
        }
        
       return true  // returns bool
    }
    
    func spawnBlock() -> SKSpriteNode? {
        
        let maxBlockCount: Int = 5
        if self.blockCount < maxBlockCount {
            
            var newBlock = createBlock()
            var xOffset = UInt32(newBlock.size.width/2)
            var maxX = UInt32(CGRectGetMaxX(self.frame)) - UInt32(newBlock.size.width)
            var maxY = UInt32(CGRectGetMaxY(self.frame) * 0.5) - UInt32(newBlock.size.height/2)
            var spawnX = xOffset + arc4random_uniform(maxX)
            let yOffset = UInt32(self.frame.height/2)
            var spawnY = arc4random_uniform(maxY) + yOffset  // min height at mid screen height
            newBlock.position = CGPoint(x: Double(spawnX), y: Double(spawnY))
            // println("NewBlock: \(newBlock.position)")
            
            if (testIfValidBlock(newBlock)) {
           
            //self.addChild(newBlock)
            self.blockCount += 1
                println(newBlock.position)
                println(self.blockCount)
            return newBlock
        
            }
        }
        return nil
    }
    
    
/*    func spawnBlocksinContainer() -> SKNode {
        
        let container: SKNode = SKNode()

        let action = SKAction.runBlock ({ () -> Void in
            var block = self.spawnBlock()
            if block != nil {
                container.addChild(block!)
            }
        })
        
        container.runAction(SKAction.repeatActionForever(action))
        
        return container
    }   */

    
    override func touchesMoved(touches: NSSet, withEvent event: UIEvent) {
        
        for i in touches {
            
            var touch: UITouch? = i as? UITouch
            if touch != nil {
                
                var touchPt: CGPoint = touch!.locationInNode(self)
                var node: SKNode = self.nodeAtPoint(touchPt)
                if node.name == "ball" {
                    
                    
                    // set class variables
                    // println("ball")
                    self.endTouch = touch!
                    self.endTouchPt = touchPt
                    self.endTime = touch!.timestamp
                    break
                }
            
            }
        }
       
        if (self.startTouch != nil && self.endTouch != nil && self.endTouchPt != nil && self.canHit == false) {  // if user has performed a swipe and ball has not been hit
        var dx: CGFloat = self.endTouchPt!.x - self.startTouchPt!.x
        var dy: CGFloat = self.endTouchPt!.y - self.startTouchPt!.y
        self.endTime = self.endTouch!.timestamp
        var dTime =  endTime - self.startTime
        
        self.dxScale = Double(dx) / Double(dTime)
        self.dyScale = Double(dy) / Double(dTime)
        self.canHit = true  // set to true so that update can apply force
        //println(self.startTime)
        //println(dxScale)
            
        
        // apply impulse to children
         /*   if self.canHit {
        var ball = self.childNodeWithName("ball")
        var xForce = CGFloat(dxScale) * scalingFactor
        var yForce = CGFloat(dyScale) * scalingFactor
        ball!.physicsBody?.applyForce(CGVectorMake(xForce, yForce), atPoint: self.endTouchPt!)
        self.canHit = false
        self.startTouch = nil
        self.endTouch = nil
        self.endTouchPt = nil
            } */

        }
        
    }
    
    override func touchesEnded(touches: NSSet, withEvent event: UIEvent) {
        
        
       self.canHit = true
        println(self.canHit)
        
        /* var touch = touches.anyObject() as? UITouch
        self.endTouch = touch
        self.endTouchPt = touch?.locationInNode(self)
        if (self.startTouch != nil && self.endTouch != nil) {
            
            var dx: CGFloat = self.endTouchPt!.x - self.startTouchPt!.x
            var dy: CGFloat = self.endTouchPt!.y - self.startTouchPt!.y
            self.endTime = self.endTouch!.timestamp
            var dTime =  endTime - self.startTime
            let scalingFactor: CGFloat = 10 // to scale vector quantity by set amount
            var dxScale = Double(dx) / Double(dTime)
            var dyScale = Double(dy) / Double(dTime)
            //println(self.startTime)
            //println(dxScale)
            var projectile = self.createHitProjectile()
            projectile.position = self.endTouchPt!
            
            // add children here
            self.addChild(projectile)
            
            
            // apply impulse to children
            projectile.physicsBody?.applyImpulse(CGVectorMake(CGFloat(dxScale), CGFloat(dyScale)))
        } */
        
        
        
    }
   
    override func update(currentTime: CFTimeInterval) {
        /* Called before each frame is rendered */
        
        // basically update is the game loop called every frame
        
        if self.canHit && self.endTouchPt != nil { // prevent crashing during initial part
            
            var xForce = CGFloat(self.dxScale) * scalingFactor
            var yForce = CGFloat(self.dyScale) * scalingFactor
            var ball = self.childNodeWithName("ball")
            ball!.physicsBody?.applyForce(CGVectorMake(xForce, yForce), atPoint: self.endTouchPt!)
            self.ballKicked = true
            self.endTouchPt = nil
            self.canHit = false  // ensure no double hitting within same swipe
        }
        
        
        
        var block = self.spawnBlock()
        if block != nil {
            self.addChild(block!)
        }

        
        if self.ball.physicsBody?.velocity.dy == 0 {  // to test how to read ball position and how to set parameters
            var heightShift: CGFloat = 100
            //self.ball.position = CGPointMake(self.midX, self.maxHeight - heightShift)
            // self.ball.physicsBody?.applyForce(CGVectorMake(0, 2000))
        }
    }
}
