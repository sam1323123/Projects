//
//  CalculatorViewController.swift
//  Midterm Project- Finance app
//
//  Created by Samuel Lee on 2/15/15.
//  Copyright (c) 2015 Samuel Lee. All rights reserved.
//

import UIKit

class CalculatorViewController: UIViewController {
    
    var screenRect: CGRect = UIScreen.mainScreen().bounds
    let numbersArray: [String] = ["1", "2", "3", "+", "4", "5", "6", "-", "7", "8", "9", "*", "0", ".", "Exp", "/", "Clear", "Del", "="]
    // let opArray: [String] = ["+", "-", "*", "/"]
    var display = UITextView(frame: CGRect(origin: CGPointZero, size: CGSize(width: 0 , height: 0)))
    var changingNum: String = ""
    var storedNum: String = ""
    var operand: String = ""
    var answer: String = ""
    var onSameNum: Bool = true
    var prevOp = ""
    var equalsPressed: Bool = false
    
    
    func drawButtons() {
        self.drawTextField()
        var navBarHeight: CGFloat = 60
        var xMargin : CGFloat = 5
        var bottomMargin: CGFloat = 10
        var numStartX = xMargin
        var numStartY = (CGRectGetHeight(screenRect) - bottomMargin - navBarHeight) * 0.2 + navBarHeight
        var totalButtonHeight = (CGRectGetHeight(screenRect) - bottomMargin - navBarHeight) * 0.8
        var buttonHeight = totalButtonHeight / 5
        var buttonWidth = (CGRectGetWidth(self.screenRect) - 2 * xMargin) / 4
        // start of number buttons
        for numIndex in 0...(self.numbersArray.count-1) {
            var str = self.numbersArray[numIndex]
            var char: Int? = str.toInt()
            var newButton: UIButton
            if char != nil {
            newButton  = buttonFactory.buttonWithType(buttonType.number, label: self.numbersArray[numIndex])
            }
            else if self.numbersArray[numIndex] == "="{
                    newButton = buttonFactory.buttonWithType(buttonType.equal, label: self.numbersArray[numIndex])
                // var eqButtonHeight: CGFloat = CGRectGetHeight(newButton.bounds)
                var eqbuttonWidth = (CGRectGetWidth(self.screenRect) - 2 * xMargin) - (2 * buttonWidth)
                newButton.bounds = CGRect(origin: CGPointZero, size: CGSize(width: eqbuttonWidth, height:buttonHeight))
                var spaceX : CGFloat = CGFloat(numIndex % 4)
                var spaceY: CGFloat = CGFloat(numIndex / 4)
                var centreX = (numStartX + (buttonWidth * spaceX)) + (eqbuttonWidth/2)
                // println(centreX)
                var centreY = (numStartY + (spaceY * buttonHeight)) + (buttonHeight/2)
                newButton.center = CGPoint(x: centreX, y: centreY)
                newButton.addTarget(self, action: "storeValuesToSameNum:", forControlEvents: UIControlEvents.TouchUpInside)
                self.view.addSubview(newButton)
                continue
                }
            else {
                newButton  = buttonFactory.buttonWithType(buttonType.op, label: self.numbersArray[numIndex])
            }
                // newButton.tag = numIndex
                // var buttonSize: CGFloat = CGRectGetHeight(newButton.bounds)
                var spaceX : CGFloat = CGFloat(numIndex % 4)
                var spaceY: CGFloat = CGFloat(numIndex / 4)
                newButton.bounds = CGRect(origin: CGPointZero, size: CGSize(width: buttonWidth, height: buttonHeight))
                var centreX = (numStartX + (buttonWidth * spaceX)) + (buttonWidth/2)
                // println(centreX)
                var centreY = (numStartY + (spaceY * buttonHeight)) + (buttonHeight/2)
                newButton.center = CGPoint(x: centreX, y: centreY)
                newButton.addTarget(self, action: "storeValuesToSameNum:", forControlEvents: UIControlEvents.TouchUpInside)
                self.view.addSubview(newButton)
                
                
            }
        
            
            }
    

    
    func drawTextField() {
        let startY = 60
        let navBarHeight: CGFloat = 60
        let bottomMargin: CGFloat = 10
        let textViewFrame = CGRect(origin: CGPoint(x: 0, y: startY), size: CGSize(width: CGRectGetWidth(self.screenRect), height: (CGRectGetHeight(screenRect) - bottomMargin - navBarHeight) * 0.2))
        self.display = UITextView(frame: textViewFrame)
        self.display.textAlignment = NSTextAlignment(rawValue: 2)! // right alignment of text
        self.display.backgroundColor = UIColor.whiteColor()
        self.display.tag = 01 // 01 designation
        self.display.textColor = UIColor.redColor()
        self.display.font = UIFont(name: "Verdana", size: 14.0)
        self.display.layer.borderWidth = 1.5
        self.display.layer.borderColor = UIColor.blackColor().CGColor
        self.display.editable = false
        self.view.addSubview(display)
        
    }
    
    override func didRotateFromInterfaceOrientation(fromInterfaceOrientation: UIInterfaceOrientation) {
        for view in self.view.subviews {
            view.removeFromSuperview()
        }
        self.updateInterface()
    }
    
    func updateInterface() {
        self.screenRect = UIScreen.mainScreen().bounds
        self.drawButtons()
        self.display.text = "\(self.answer)\n\(self.changingNum)"
    }
    
    func checkIfNumOrDotOrClearOrDel(but: UIButton) -> Bool {
        for num in 0...9 {
            if but.currentTitle! == String(num) {
                return true
            }
        
        }
        if but.currentTitle! == "." { return true}
        else if but.currentTitle! == "Clear" { return true }
        else if but.currentTitle! == "Del" {return true}
        return false
    }
    
    func storeValuesToSameNum(sender: UIButton) {
        if self.checkIfNumOrDotOrClearOrDel(sender) { // if user pressed number
            if sender.currentTitle! == "Clear" {
                self.answer = ""
                self.changingNum = ""
                self.storedNum = ""
                self.operand = ""
            }
            else if sender.currentTitle! == "Del" {
                if countElements(self.changingNum) > 0 {
                var cutlength = countElements(self.changingNum) - 1
                self.changingNum = self.changingNum.substringToIndex(advance(self.changingNum.startIndex, cutlength))
                }
            }
            else if sender.currentTitle! == "." {
                if self.changingNum.rangeOfString(".") != nil {
                    self.changingNum += ""
                }
                else { self.changingNum += "." }
            }
            else {
            self.onSameNum = true
            self.changingNum += sender.currentTitle!
            // add to currentNum String
            }
        
        }
        else { // operand is pressed
            self.onSameNum = false   // and operation is pressed
            if sender.currentTitle! == "=" {
                if self.storedNum != "" && self.changingNum != "" {
                    if self.operand == "" { self.operand = self.prevOp }
                self.performCalculation()
                self.equalsPressed = true
                self.display.text = "\(self.answer)\n"
                    return
                }
            }
            println("operand pressed")
            if self.operand == "" && self.prevOp == "" {
               self.prevOp = sender.currentTitle!
                }
            if self.equalsPressed == true && sender.currentTitle! != "=" {
                self.equalsPressed = false
                self.operand = sender.currentTitle!
            }
            if self.storedNum == "" { // for first case
            self.storedNum = self.changingNum // store for operation later
            self.changingNum = ""
            }
            
        }
        if self.storedNum != "" && self.changingNum != "" && self.answer == ""  { // after 2 numbers entered
            if self.onSameNum == false {
            self.operand = self.prevOp
            self.performCalculation()
            self.operand = sender.currentTitle!
            }
        }
        else if self.storedNum != "" && self.changingNum != "" && self.answer != ""  { // self.answer has a value, transfer to stored value for operation
            self.storedNum = self.answer
            if self.onSameNum == false {
                self.performCalculation()
                self.operand = sender.currentTitle!
            }
        }
        //if self.answer != "" {
        if self.answer == "" {
            self.display.text = "\(self.storedNum)\n\(self.changingNum)"
        }
        else{
        self.display.text = "\(self.answer)\n\(self.changingNum)"
        }
        //println(self.answer)
       // println(self.changingNum) 
      //  }
       // else { // no answer yet
         //   self.display.text = "\n\(self.changingNum)"
        //}
    }
    
    
    func performCalculation() {
       // println(self.changingNum); println(self.storedNum)
       // var num1 = NSNumberFormatter().numberFromString(self.storedNum)?.floatValue
        // var num2 = NSNumberFormatter().numberFromString(self.changingNum)?.floatValue
        let maxValue = Int.max
        
        if self.answer == "Error Overflow" {  // reaches mav value
            return
        }
        var num1 = NSString(string: self.storedNum).floatValue
        var num2 = NSString(string: self.changingNum).floatValue
        switch self.operand {
        case "+":
            var result = num1 + num2
            if round(result) == result && result <= (Float(maxValue)) {
                var result = Int(result)
                //println("converted \(result) to Int")
                self.answer = "\(result)"
                break
            }
            self.answer = "\(result)"
            
        case "-":
            var result = num1 - num2
            if round(result) == result && result <= (Float(maxValue)) {
                var result = Int(result)
                //println("converted \(result) to Int")
                self.answer = "\(result)"
                break
            }
            self.answer = "\(result)"
            
        case "*":
            var result = (num1 * 1.0) * num2
            if round(result) == result && result <= (Float(maxValue)) {
                var result = Int(result)
                //println("converted \(result) to Int")
                self.answer = "\(result)"
                break}
            self.answer = "\(result)"
            
        case "/":
            var result = (num1 * 1.0) / num2
            //println("result: \(result)")
            if round(result) == result && result <= (Float(maxValue)) {
                var result = Int(result)
                //println("converted \(result) to Int")
                self.answer = "\(result)"
                break
            
            }
            self.answer = "\(result)"
                //NSNumberFormatter().stringFromNumber(result)!
        
            
        case "Exp":
            var result = pow(num1, num2)
            if round(result) == result && result <= (Float(maxValue)) {
                var result = Int(result)
                //println("converted \(result) to Int")
                self.answer = "\(result)"
                break
                
            }
            self.answer = "\(result)"
            
            
        case "":
            break
            
        
        default:
            break
            
        }
        self.changingNum = ""
        if NSString(string: self.answer).doubleValue >= (Double(maxValue)) {
            self.answer = "Error Overflow"
        }
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        self.drawButtons()

        // Do any additional setup after loading the view.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject?) {
        // Get the new view controller using segue.destinationViewController.
        // Pass the selected object to the new view controller.
    }
    */

}
